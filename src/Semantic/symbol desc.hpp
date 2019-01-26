//
//  symbol desc.hpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_symbol_desc_hpp
#define stela_symbol_desc_hpp

#include "symbols.hpp"

namespace stela {

std::string_view symbolDesc(const sym::Symbol *);
std::string typeDesc(const ast::TypePtr &);

}

#endif
