#pragma once

#include "llvm/IR/DerivedTypes.h"

#include "ast.hpp"

llvm::Value *getNamedValues(std::string);
void setNamedValues(std::string, llvm::Value *);
void clearNamedValues();

void setFunctionProtos(std::string, std::shared_ptr<PrototypeAST>);
std::shared_ptr<PrototypeAST> findFunctionProtos(std::string);
