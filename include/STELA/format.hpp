//
//  format.hpp
//  STELA
//
//  Created by Indi Kernick on 14/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_format_hpp
#define stela_format_hpp

#include <vector>
#include "log.hpp"
#include <string_view>

namespace stela::fmt {

enum class Tag {
  plain,
  oper,
  string,
  character,
  number,
  keyword,
  type_name,
  member,
  
  newline,
  indent
};

struct Token {
  union {
    // valid if tag is Tag::newline or Tag::indent
    uint32_t count;
    std::string_view text;
  };
  Tag tag;
};

using Tokens = std::vector<Token>;

}

namespace stela {

fmt::Tokens format(std::string_view, LogBuf &);

}

#endif
