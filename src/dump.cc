#include <iostream>

#include "ast.hpp"

void NumberExprAST::dump(std::string s) {
  std::cout << s;
  std::cout << "NumberExprAST with value: <" << Val << ">" << std::endl;
}
void VariableExprAST::dump(std::string s) {
  std::cout << s;
  std::cout << "VariableExprAST with name: <" << Name << ">" << std::endl;
}

void BinaryExprAST::dump(std::string s) {
  std::cout << s;
  std::cout << "BinaryExprAST with Op: <";
  if (Op == 1)
    std::cout << "+>" << std::endl;
  if (Op == 2)
    std::cout << "->" << std::endl;
  if (Op == 3)
    std::cout << "*>" << std::endl;
  if (Op == 4)
    std::cout << "/>" << std::endl;
  std::cout << "  ";
  LHS->dump(s + s);
  RHS->dump(s + s);
}

void CallExprAST::dump(std::string s) {
  std::cout << s;
  std::cout << "CallExprAST with Callee: <" << Callee << ">" << std::endl;
  for (auto &&x : Args) {
    std::cout << s << "arg: " << std::endl;
    x->dump(s + s);
  }
}

void PrototypeAST::dump(std::string s) {
  std::cout << s;
  std::cout << "PrototypeAST with name: <" << Name << ">";
  std::cout << " and args: <";
  for (auto &&x : Args) {
    std::cout << x << ", ";
  }
  std::cout << ">" << std::endl;
}
void FunctionAST::dump(std::string s) {
  std::cout << "FunctionAST" << std::endl;
  Proto->dump();
  Body->dump("    ");
}
