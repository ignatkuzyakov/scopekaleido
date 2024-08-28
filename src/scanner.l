%option c++
%option noyywrap

%{

#include "grammar.tab.hh"

%}

WS      [ \t\n\v]+
DIGIT   [0-9]
DIGIT1  [1-9]
id      [a-zA-Z][a-zA-Z0-9]*

%%

{WS}                  /* skip blanks and tabs */
"def"                 return yy::parser::token_type::DEF;
"quit"                return yy::parser::token_type::QUIT;
"extern"              return yy::parser::token_type::EXTERN;
"+"                   return yy::parser::token_type::PLUS;
"-"                   return yy::parser::token_type::MINUS;
";"                   return yy::parser::token_type::SCOLON;
"*"                   return yy::parser::token_type::MUL;
"/"                   return yy::parser::token_type::DIV;
"("                   return yy::parser::token_type::LBR;
")"                   return yy::parser::token_type::RBR;
","                   return yy::parser::token_type::COMMA;
{DIGIT1}{DIGIT}*    | 
{DIGIT}+"."{DIGIT}* | 
"."{DIGIT}*           return yy::parser::token_type::NUMBER;
{id}                  return yy::parser::token_type::ID;
.	                    return yy::parser::token_type::ERR;

%%

