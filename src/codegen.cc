#include "KaleidoscopeJIT.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include <cstdio>
#include <memory>
#include <string>

#include "ast.hpp"
#include "codegen.hpp"
#include "symtab.hpp"

using namespace llvm;
using namespace llvm::orc;

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;

static std::unique_ptr<KaleidoscopeJIT> TheJIT;
static std::unique_ptr<FunctionPassManager> TheFPM;
static std::unique_ptr<LoopAnalysisManager> TheLAM;
static std::unique_ptr<FunctionAnalysisManager> TheFAM;
static std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
static std::unique_ptr<ModuleAnalysisManager> TheMAM;
static std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
static std::unique_ptr<StandardInstrumentations> TheSI;

static std::map<std::string, llvm::Value *> NamedValues;
static std::map<std::string, std::shared_ptr<PrototypeAST>> FunctionProtos;

static ExitOnError ExitOnErr;

Value *LogErrorV(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  return nullptr;
}

llvm::Value *getNamedValues(std::string name) { return NamedValues[name]; }
void setNamedValues(std::string name, llvm::Value *arg) {
  NamedValues[name] = arg;
}

void clearNamedValues() { NamedValues.clear(); }

void setFunctionProtos(std::string name, std::shared_ptr<PrototypeAST> proto) {
  FunctionProtos[name] = proto;
}

std::shared_ptr<PrototypeAST> findFunctionProtos(std::string Name) {
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second;
  return nullptr;
}

Value *NumberExprAST::codegen() {
  return ConstantFP::get(*TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen() {
  Value *V = getNamedValues(Name);
  if (!V)
    return LogErrorV("unknown variable name");
  return V;
}

Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op) {
  case '+':
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '/':
    return Builder->CreateFDiv(L, R, "divtmp");
  case '<':
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

Function *getFunction(std::string Name) {
  if (auto *F = TheModule->getFunction(Name))
    return F;

  if (auto r = findFunctionProtos(Name))
    return r->codegen();

  return nullptr;
}

Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  Function *CalleeF = getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::codegen() {
  // Make the function type:  double(double,double) etc.
  std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
  FunctionType *FT =
      FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);

  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}

Function *FunctionAST::codegen() {
  setFunctionProtos(Proto->getName(), Proto);
  Function *TheFunction = getFunction(Proto->getName());
  if (!TheFunction)
    return nullptr;

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  clearNamedValues();
  for (auto &Arg : TheFunction->args())
    setNamedValues(std::string(Arg.getName()), &Arg);

  if (Value *RetVal = Body->codegen()) {

    Builder->CreateRet(RetVal);
    verifyFunction(*TheFunction);

    TheFPM->run(*TheFunction, *TheFAM);
    return TheFunction;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}

void HandleDefinition(llvm::Function *fp) {
  if (!fp) {
    fprintf(stderr, "incorrect function pointer\n");
    return;
  }
  fp->print(errs());
  ExitOnErr(TheJIT->addModule(
      ThreadSafeModule(std::move(TheModule), std::move(TheContext))));
  InitializeModuleAndManagers();
}
void HandleExtern(llvm::Function *fp) {
  if (!fp) {
    fprintf(stderr, "incorrect function pointer\n");
    return;
  }
  fp->print(errs());
}
void HandleTopLevelExpression(llvm::Function *fp) {
  if (!fp) {
    fprintf(stderr, "incorrect function pointer\n");
    return;
  }
  auto RT = TheJIT->getMainJITDylib().createResourceTracker();
  auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));

  ExitOnErr(TheJIT->addModule(std::move(TSM), RT));

  InitializeModuleAndManagers();

  auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));

  double (*FP)() = ExprSymbol.getAddress().toPtr<double (*)()>();

  fprintf(stderr, "Evaluated to %f\n", FP());
  ExitOnErr(RT->remove());
}

void CreateJIT() { TheJIT = ExitOnErr(KaleidoscopeJIT::Create()); }

void InitializeTargets() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
}

void InitializeModuleAndManagers() {
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("KaleidoscopeJIT", *TheContext);
  TheModule->setDataLayout(TheJIT->getDataLayout());
  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Create new pass and analysis managers
  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();

  // Required for the pass instrumentation framework,
  // which allows developers to customize what happens between passes.
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext,
                                                     /*DebugLogging*/ true);

  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add transform passes.
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->addPass(InstCombinePass());
  // Reassociate expressions.
  TheFPM->addPass(ReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->addPass(GVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->addPass(SimplifyCFGPass());

  // Register analysis passes used in these transform passes.
  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
}
