//
//  check scopes.cpp
//  STELA
//
//  Created by Indi Kernick on 27/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "check scopes.hpp"

#include <cassert>
#include "symbol desc.hpp"

using namespace stela;

namespace {

void unusedSymbols(Log &log, const sym::Scopes &scopes) {
  assert(scopes.size() > 1);
  for (auto s = scopes.cbegin() + 1; s != scopes.cend(); ++s) {
    const sym::Scope &scope = *(*s);
    if (!scope.module.empty()) {
      log.module(scope.module);
    }
    for (const auto &pair : scope.table) {
      const sym::Symbol *sym = pair.second.get();
      if (!sym->referenced) {
        log.warn(sym->loc) << "Unused " << symbolDesc(sym) << " \""
          << pair.first << '"' << endlog;
      }
    }
  }
}

}

void stela::checkScopes(Log &log, const Symbols &syms) {
  unusedSymbols(log, syms.scopes);
}
