//
//  lookup.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lookup.hpp"

using namespace stela;

sym::Symbol *stela::lookupDup(
  const sym::Table &table,
  const std::string_view name
) {
  const auto iter = table.find(name);
  if (iter == table.end()) {
    return nullptr;
  }
  return iter->second.get();
}

sym::Symbol *stela::lookupUse(
  const sym::Scope &scope,
  const std::string_view name,
  const Loc loc,
  Log &log
) {
  const auto iter = scope.table.find(name);
  if (iter == scope.table.end()) {
    if (scope.parent) {
      return lookupUse(*scope.parent, name, loc, log);
    } else {
      log.ferror(loc) << "Use of undefined symbol \"" << name << '"' << endlog;
    }
  }
  iter->second->referenced = true;
  return iter->second.get();
}

sym::Symbol *stela::lookupDup(
  const sym::Table &table,
  const std::string_view name,
  const sym::FuncParams &params
) {
  const auto [begin, end] = table.equal_range(name);
  if (begin == end) {
    return nullptr;
  }
  for (auto i = begin; i != end; ++i) {
    sym::Func *func = dynamic_cast<sym::Func *>(i->second.get());
    if (func == nullptr) {
      // redefinition of 'name' as a different kind of symbol
      return i->second.get();
    } else {
      if (func->params == params) {
        // redefinition of 'name'
        return i->second.get();
      }
    }
  }
  return nullptr;
}

sym::Func *stela::lookupUse(
  const sym::Scope &scope,
  const std::string_view name,
  const sym::FuncParams &params,
  const Loc loc,
  Log &log
) {
  const auto [begin, end] = scope.table.equal_range(name);
  if (begin == end) {
    if (scope.parent) {
      return lookupUse(*scope.parent, name, params, loc, log);
    } else {
      log.ferror(loc) << "Use of undefined symbol \"" << name << '"' << endlog;
    }
  } else if (std::next(begin) == end) {
    sym::Func *func = dynamic_cast<sym::Func *>(begin->second.get());
    if (func == nullptr) {
      // @TODO check if this is a function object
      log.ferror(loc) << "Calling \"" << name
        << "\" but it is not a function. " << begin->second->loc << endlog;
    } else {
      if (func->params == params) {
        return func;
      } else {
        // @TODO lookup type conversions
        log.ferror(loc) << "No matching call to function \"" << name
          << "\" at " << func->loc << endlog;
      }
    }
  } else {
    for (auto i = begin; i != end; ++i) {
      sym::Func *func = dynamic_cast<sym::Func *>(i->second.get());
      // if there is more than one symbol with the same name then those symbols
      // must be functions
      assert(func);
      if (func->params == params) {
        return func;
      }
    }
    log.ferror(loc) << "Ambiguous call to function \"" << name << '"' << endlog;
  }
  return nullptr;
}
