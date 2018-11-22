//
//  scope traverse.hpp
//  STELA
//
//  Created by Indi Kernick on 19/8/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_traverse_hpp
#define stela_scope_traverse_hpp

#include "symbols.hpp"

namespace stela {

inline sym::Scope *findNearest(sym::ScopeType type, sym::Scope *scope) {
  if (scope == nullptr) {
    return nullptr;
  } else if (scope->type == type) {
    return scope;
  } else {
    return findNearest(type, scope->parent);
  }
}

inline sym::Scope *findNearestEither(sym::ScopeType a, sym::ScopeType b, sym::Scope *scope) {
  if (scope == nullptr) {
    return nullptr;
  } else if (sym::ScopeType type = scope->type; type == a || type == b) {
    return scope;
  } else {
    return findNearestEither(a, b, scope->parent);
  }
}

inline sym::Scope *findNearestNot(sym::ScopeType type, sym::Scope *scope) {
  if (scope == nullptr) {
    return nullptr;
  } else if (scope->type == type) {
    return findNearestNot(type, scope->parent);
  } else {
    return scope;
  }
}

}

#endif
