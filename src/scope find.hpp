//
//  scope find.hpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_find_hpp
#define stela_scope_find_hpp

#include "symbols.hpp"

namespace stela {

sym::Symbol *find(sym::Scope *, sym::Name);
std::vector<sym::Symbol *> findMany(sym::Scope *, sym::Name);

}

#endif
