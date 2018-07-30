//
//  scope lookup.hpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_lookup_hpp
#define stela_scope_lookup_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "log output.hpp"

namespace stela {

sym::Symbol *lookup(sym::Scope *, Log &, sym::Name, Loc);
sym::Func *lookup(sym::Scope *, Log &, const sym::FunKey &, Loc);

sym::Symbol *lookup(sym::StructScope *, Log &, const sym::MemKey &, Loc);
sym::Func *lookup(sym::StructScope *, Log &, const sym::MemFunKey &, Loc);

sym::Symbol *type(sym::Scope *, Log &, const ast::TypePtr &);
sym::FuncParams funcParams(sym::Scope *, Log &, const ast::FuncParams &);

}

#endif
