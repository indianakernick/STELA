//
//  scope manager.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope manager.hpp"

#include <cassert>
#include "compare params args.hpp"

using namespace stela;

ScopeMan::ScopeMan(sym::Scopes &scopes)
  : scopes{scopes} {
  assert(!scopes.empty());
  scope = scopes.back().get();
}

void ScopeMan::leaveScope() {
  assert(scope);
  scope = scope->parent;
}

sym::Scope *ScopeMan::cur() const {
  return scope;
}

sym::Scope *ScopeMan::par() const {
  assert(scope);
  return scope->parent;
}
