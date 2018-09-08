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

inline sym::Scope *findNearest(const sym::Scope::Type type, sym::Scope *const scope) {
  if (scope == nullptr) {
    return nullptr;
  } else if (scope->type == type) {
    return scope;
  } else {
    return findNearest(type, scope->parent);
  }
}

inline sym::Scope *findNearestNot(const sym::Scope::Type type, sym::Scope *const scope) {
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
