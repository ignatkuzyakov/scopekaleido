%language "c++"

%skeleton "lalr1.cc"
%defines
%define api.value.type {struct semantic_value_type}
%param {yy::Driver* driver}

%code requires
{
#include <algorithm>
#include <string>
#include <vector>
#include <memory>

#include "ast.hpp"
#include "codegen.hpp"
#include "symtab.hpp"

struct semantic_value_type {
  std::string name;
  double value;

  std::shared_ptr<PrototypeAST> prot;
  std::shared_ptr<FunctionAST> func;
  std::shared_ptr<ExprAST> tree;

  std::vector<std::shared_ptr<ExprAST>> exprs;
  std::vector<std::string> args;
};

// forward decl of argument to parser
namespace yy { class Driver; }
}

%code
{
#include "driver.hpp"

namespace yy {

parser::token_type yylex(parser::semantic_type* yylval,                         
                         Driver* driver);
}
}

%token ID
%token NUMBER 
%token EXTERN DEF QUIT

%token
  LBR     "("
  RBR     ")"
  MINUS   "-"
  PLUS    "+"
  MUL     "*"
  DIV     "/"
  SCOLON  ";"
  COMMA   ","
  ERR
;

%left '+' '-'
%left '*' '/'

%start top

%%

  top:            definition SCOLON         { HandleDefinition($1.func->codegen());           return 1; }
                | external   SCOLON         { 
                                              HandleExtern($1.prot->codegen()); 
                                              setFunctionProtos($1.prot->getName(), $1.prot); return 1;
                                            }
                | toplvlexpr SCOLON         { HandleTopLevelExpression($1.func->codegen());   return 1; }
                | QUIT                      {                                                 return 2; }
                

  external:       EXTERN prototype          { $$.prot = $2.prot; }

  definition:     DEF prototype expr        { $$.func = std::make_shared<FunctionAST>($2.prot, $3.tree); }

  prototype:      ID LBR protargs RBR       { $$.prot = std::make_shared<PrototypeAST>($1.name, $3.args); }
      
  protargs:       /*empty*/                 { $$.args = std::vector<std::string>();                     }   
                | protargs COMMA ID         { $$.args = $1.args; $$.args.push_back($3.name); }
                | ID                        { $$.args.push_back($1.name);                               }
  

  toplvlexpr:     expr                      { 
                                              auto p = std::make_shared<PrototypeAST>("__anon_expr", std::vector<std::string>());
                                              $$.func = std::make_shared<FunctionAST>((p), $1.tree); 
                                            }

  call:           ID LBR callargs RBR       { $$.tree = std::make_shared<CallExprAST>($1.name, $3.exprs); }

  callargs:      /*empty*/                  {}
                | callargs COMMA expr       { $$.exprs = $1.exprs; $$.exprs.push_back($3.tree);}
                | expr                      { $$.exprs.push_back($1.tree);} 

  expr:           expr PLUS  term           { $$.tree = std::make_shared<BinaryExprAST>('+', $1.tree, $3.tree); }
                | expr MINUS term           { $$.tree = std::make_shared<BinaryExprAST>('-', $1.tree, $3.tree); }
                | term                      { $$.tree = $1.tree;  }
  
  term:           term MUL factor           { $$.tree = std::make_shared<BinaryExprAST>('*', $1.tree, $3.tree); }
                | term DIV factor           { $$.tree = std::make_shared<BinaryExprAST>('/', $1.tree, $3.tree); }
                | factor                    { $$.tree = $1.tree;  }
                | call                      { $$.tree = $1.tree;  }

  factor:         LBR expr RBR              { $$.tree = $2.tree;  }
                | NUMBER                    { $$.tree = std::make_shared<NumberExprAST>($1.value);  }
                | ID                        { $$.tree = std::make_shared<VariableExprAST>($1.name); }

%%

namespace yy {

parser::token_type yylex(parser::semantic_type* yylval,                         
                         Driver* driver)
{
  return driver->yylex(yylval);
}

void parser::error(const std::string&){}
}
