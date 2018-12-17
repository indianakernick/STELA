//
//  plain format.hpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_plain_format_hpp
#define stela_plain_format_hpp

#include <string>
#include <iosfwd>
#include "format.hpp"

namespace stela {

std::string plainFormat(const fmt::Tokens &, uint32_t = 2);
void plainFormat(std::ostream &, const fmt::Tokens &, uint32_t = 2);

std::string plainFormatInline(const fmt::Tokens &);
void plainFormatInline(std::ostream &, const fmt::Tokens &);

}

#endif
