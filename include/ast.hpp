#pragma once

#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() = default;

  virtual void dump(std::string = "") = 0;

  virtual llvm::Value *codegen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}

  virtual void dump(std::string) override;

  virtual llvm::Value *codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}

  virtual void dump(std::string) override;

  virtual llvm::Value *codegen() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  std::shared_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char Op, std::shared_ptr<ExprAST> LHS,
                std::shared_ptr<ExprAST> RHS)
      : Op(Op), LHS(LHS), RHS(RHS) {}

  virtual void dump(std::string) override;

  virtual llvm::Value *codegen() override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::shared_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::shared_ptr<ExprAST>> Args)
      : Callee(Callee), Args(Args) {}

  virtual void dump(std::string) override;

  virtual llvm::Value *codegen() override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args)
      : Name(Name), Args(Args) {}

  const std::string &getName() const { return Name; }

  virtual void dump(std::string = "");

  virtual llvm::Function *codegen();
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::shared_ptr<PrototypeAST> Proto;
  std::shared_ptr<ExprAST> Body;

public:
  FunctionAST(std::shared_ptr<PrototypeAST> Proto,
              std::shared_ptr<ExprAST> Body)
      : Proto(Proto), Body(Body) {}

  virtual void dump(std::string = "");

  virtual llvm::Function *codegen();
};
