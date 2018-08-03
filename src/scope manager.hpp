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
    Scope *const newScope = scopes.emplace_back(std::make_unique<Scope>(scope));
    scope = newScope;
    return newScope;
  }
  void leaveScope();
  sym::Scope *cur() const;
  sym::Scope *par() const;
  sym::Scope *builtin() const;
  sym::Scope *global() const;

private:
  sym::Scopes &scopes;
  sym::Scope *scope;
};

}

#endif
