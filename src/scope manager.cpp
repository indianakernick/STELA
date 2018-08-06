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
  assert(scopes.size() >= 2);
  scope = scopes.back().get();
}

void ScopeMan::leaveScope() {
  assert(scope);
  scope = scope->parent;
}

sym::Scope *ScopeMan::cur() const {
  return scope;
}

namespace {

sym::NSScope *assertNS(sym::Scope *const scope) {
  sym::NSScope *const ns = dynamic_cast<sym::NSScope *>(scope);
  assert(ns);
  return ns;
}

}

sym::NSScope *ScopeMan::builtin() const {
  return assertNS(scopes[0].get());
}

sym::NSScope *ScopeMan::global() const {
  return assertNS(scopes[1].get());
}
