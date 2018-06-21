//
//  lexical analysis.hpp
//  STELA
//
//  Created by Indi Kernick on 17/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_lexical_analysis_hpp
#define stela_lexical_analysis_hpp

#include "token.hpp"
#include <stdexcept>

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

Tokens lex(std::string_view);

}

#endif
