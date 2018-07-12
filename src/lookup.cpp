//
//  lookup.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lookup.hpp"

using namespace stela;

sym::Symbol *stela::lookupDup(const sym::Table &table, const std::string &name) {
  const auto iter = table.find(name);
  if (iter == table.end()) {
    return nullptr;
  }
  return iter->second.get();
}

sym::Symbol *stela::lookupUse(
  const sym::Scope &scope,
  const std::string &name,
  const Loc loc,
  Log &log
) {
  const auto iter = scope.table.find(name);
  if (iter == scope.table.end()) {
    if (scope.parent) {
      return lookupUse(*scope.parent, name, loc, log);
    } else {
      log.ferror(loc) << "Use of undefined symbol \"" << name << '"' << endlog;
      return nullptr;
    }
  }
  iter->second->referenced = true;
  return iter->second.get();
}
