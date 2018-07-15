//
//  console format.hpp
//  STELA
//
//  Created by Indi Kernick on 14/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_console_format_hpp
#define stela_console_format_hpp

#include "format.hpp"

namespace stela {

struct ConStyle {
  bool bold = false;
  bool faint = false;
  bool italic = false;
  bool underline = false;
  enum class Color {
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white
  };
  Color text = Color::white;
  Color back = Color::black;
};
using ConStyles = fmt::Styles<ConStyle>;

void conFormat(const fmt::Tokens &, const ConStyles &);
void conFormat(const fmt::Tokens &);

}

#endif
