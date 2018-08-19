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

template <typename ScopeType>
ScopeType *findNearest(sym::Scope *const scope) {
  if (scope == nullptr) {
    return nullptr;
  } else if (auto *dynamic = dynamic_cast<ScopeType *>(scope)) {
    return dynamic;
  } else {
    return findNearest<ScopeType>(scope->parent);
  }
}

template <typename... ScopeType>
sym::Scope *findNearestNot(sym::Scope *const scope) {
  if (scope == nullptr) {
    return nullptr;
  } else if ((dynamic_cast<ScopeType *>(scope) || ...)) {
    return findNearestNot<ScopeType...>(scope->parent);
  } else {
    return scope;
  }
}

}

#endif
