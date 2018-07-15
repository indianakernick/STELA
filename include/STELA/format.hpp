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

#define TAGS                                                                    \
  TAG(plain)                                                                    \
  TAG(oper)                                                                     \
  TAG(string)                                                                   \
  TAG(character)                                                                \
  TAG(number)                                                                   \
  TAG(keyword)                                                                  \
  TAG(type_name)                                                                \
  TAG(member)

enum class Tag {
  #define TAG(NAME) NAME,
  TAGS
  #undef TAG
  
  newline,
  indent
};

inline const char *tagName(const Tag tag) {
  switch (tag) {
    #define TAG(NAME) case Tag::NAME: return #NAME;
    TAGS
    #undef TAG
    default:
      return nullptr;
  }
}

struct Token {
  union {
    // valid if tag is Tag::newline or Tag::indent
    uint32_t count;
    std::string_view text;
  };
  Tag tag;
};

using Tokens = std::vector<Token>;

template <typename Style>
struct Styles {
  #define TAG(NAME) Style NAME;
  TAGS
  #undef TAG
  
  uint32_t indentSize = 2;
  
  const Style *get(const Tag tag) const {
    switch (tag) {
      #define TAG(NAME) case Tag::NAME: return &NAME;
      TAGS
      #undef TAG
      default:
        return nullptr;
    }
  }
  Style *get(const Tag tag) {
    return const_cast<Style *>(
      static_cast<const Styles *>(this)->get(tag)
    );
  }
};

#undef TAGS

}

namespace stela {

fmt::Tokens format(std::string_view, LogBuf &);

}

#endif
