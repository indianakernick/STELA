//
//  token.hpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_token_hpp
#define stela_token_hpp

#include <vector>
#include <string_view>

namespace stela {

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

}

#endif
