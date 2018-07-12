//
//  lookup.hpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_lookup_hpp
#define stela_lookup_hpp

#include "symbols.hpp"
#include "log output.hpp"

namespace stela {

sym::Symbol *lookupDup(const sym::Table &, std::string_view);
sym::Symbol *lookupUse(const sym::Scope &, std::string_view, Loc, Log &);
sym::Symbol *lookupDup(const sym::Table &, std::string_view, const sym::FuncParams &);
sym::Func *lookupUse(const sym::Scope &, std::string_view, const sym::FuncParams &, Loc, Log &);

}

#endif
