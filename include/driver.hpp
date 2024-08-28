#pragma once

#include "grammar.tab.hh"
#include <FlexLexer.h>
#include <memory>

namespace yy {

class Driver {
  std::unique_ptr<FlexLexer> plex_;

public:
  Driver() { plex_ = std::make_unique<yyFlexLexer>(); }

  parser::token_type yylex(parser::semantic_type *yylval) {
    parser::token_type tt = static_cast<parser::token_type>(plex_->yylex());
    if (tt == parser::token_type::NUMBER) {
      yylval->value = atof(plex_->YYText());
    } else if (tt == parser::token_type::ID) {
      yylval->name = plex_->YYText();
    }
    return tt;
  }

  int parse() {
    parser parser(this);
    int res = parser.parse();
    return res;
  }
};

} // namespace yy
