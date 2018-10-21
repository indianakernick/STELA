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

sym::Scope *ScopeMan::enterScope(const sym::Scope::Type type) {
  auto newScope = std::make_unique<sym::Scope>(scope, type);
  scope = newScope.get();
  scopes.push_back(std::move(newScope));
  return scope;
}

void ScopeMan::leaveScope() {
  assert(scope);
  scope = scope->parent;
}

sym::Scope *ScopeMan::cur() const {
  return scope;
}

sym::Scope *ScopeMan::builtin() const {
  assert(global()->parent);
  assert(global()->parent->type == sym::Scope::Type::ns);
  return global()->parent;
}

sym::Scope *ScopeMan::global() const {
  assert(!scopes.empty());
  assert(scopes[0]->type == sym::Scope::Type::ns);
  return scopes[0].get();
}
