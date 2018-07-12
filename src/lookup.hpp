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

/// Find a function with the same parameter types in one scope. Used for finding
/// duplicate declarations. Returns null if a matching function was not found
sym::Func *lookupDup(const sym::Table &, const std::string &, const sym::FuncParams &);
/// Find a function with compatable parameter types in current and parent scopes.
/// Used for finding functions to call. Fatal error if there are more than one
/// matching functions in one scope.
sym::Func *lookupUse(const sym::Scope &, const std::string &, const sym::FuncParams &, Log &);
/// Find a function with compatable parameter types in the current scope. Used
/// for finding functions inside structs
sym::Func *lookupMemUse(const sym::Table &, const std::string &, const sym::FuncParams &, Log &);

/// Find a symbol with the given name. Used for finding duplicate declarations.
/// If there are more than one duplicate, returns the first one encountered.
sym::Symbol *lookupDup(const sym::Table &, const std::string &);
sym::Symbol *lookupUse(const sym::Scope &, const std::string &, Loc, Log &);

}

#endif
