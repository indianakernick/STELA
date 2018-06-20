//
//  lexer.hpp
//  STELA
//
//  Created by Indi Kernick on 17/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_lexer_hpp
#define stela_lexer_hpp

#include <vector>
#include <string_view>

namespace stela {

struct InvalidToken {
  uint32_t line;
  uint32_t col;
};

struct Token {
  enum class Type {
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    STRING,
    CHARACTER,
    OPERATOR
  };

  std::string_view view;
  uint32_t line;
  uint32_t col;
  Type type;
};

std::vector<Token> lex(std::string_view);

}

#endif
