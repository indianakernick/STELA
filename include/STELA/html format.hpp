//
//  html format.hpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_html_format_hpp
#define stela_html_format_hpp

#include <string>
#include "format.hpp"

namespace stela {

struct HTMLstyles {
  uint32_t indentSize = 2;
  // generate a complete document
  bool doc = false;
  // include default stylesheet
  bool def = false;
};

std::string htmlFormat(const fmt::Tokens &, HTMLstyles);

}

#endif
