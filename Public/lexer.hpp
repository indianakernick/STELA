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
#include <stdexcept>
#include <string_view>

namespace stela {

class LexerError final : public std::runtime_error {
public:
  LexerError(uint32_t, uint32_t, const char *);
  
  uint32_t line() const;
  uint32_t col() const; 
  
private:
  uint32_t line_;
  uint32_t col_;
};

struct Token {
  enum class Type {
    keyword,
    identifier,
    number,
    string,
    character,
    oper
  };

  std::string_view view;
  uint32_t line;
  uint32_t col;
  Type type;
};

using Tokens = std::vector<Token>;

Tokens lex(std::string_view);

}

#endif
