#pragma once
#include "llvm/IR/Function.h"

void HandleDefinition(llvm::Function *);
void HandleExtern(llvm::Function *);
void HandleTopLevelExpression(llvm::Function *);

void CreateJIT();
void InitializeTargets();
void InitializeModuleAndManagers();
