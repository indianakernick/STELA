//
//  scope manager.hpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_manager_hpp
#define stela_scope_manager_hpp

#include "symbols.hpp"

namespace stela {

class ScopeMan {
public:
  explicit ScopeMan(sym::Scopes &);
  
  template <typename Scope>
  Scope *enterScope() {
    auto newScope = std::make_unique<Scope>(scope);
    Scope *const newScopePtr = newScope.get();
    scope = newScopePtr;
    scopes.push_back(std::move(newScope));
    return newScopePtr;
  }
  void leaveScope();
  sym::Scope *cur() const;
  sym::Scope *par() const;
  sym::NSScope *builtin() const;
  sym::NSScope *global() const;

private:
  sym::Scopes &scopes;
  sym::Scope *scope;
};

}

#endif
