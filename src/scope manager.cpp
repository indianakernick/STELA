//
//  scope manager.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope manager.hpp"

#include <cassert>

using namespace stela;

ScopeMan::ScopeMan(sym::Scopes &scopes, sym::Scope *scope)
  : scopes{scopes}, scope{scope} {
  assert(scope);
}

sym::Scope *ScopeMan::enterScope(sym::ScopeType type) {
  return pushScope(std::make_unique<sym::Scope>(scope, type));
}

sym::Scope *ScopeMan::enterScope(sym::ScopeType type, sym::Symbol *symbol) {
  return pushScope(std::make_unique<sym::Scope>(scope, type, symbol));
}

void ScopeMan::leaveScope() {
  assert(scope);
  scope = scope->parent;
}

sym::Scope *ScopeMan::cur() const {
  return scope;
}

sym::Scope *ScopeMan::pushScope(std::unique_ptr<sym::Scope> newScope) {
  scope = newScope.get();
  scopes.push_back(std::move(newScope));
  return scope;
}
