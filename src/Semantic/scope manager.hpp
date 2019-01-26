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
  ScopeMan(ScopeMan &&) = default;
  ScopeMan &operator=(ScopeMan &&) = default;
  
  ScopeMan(sym::Scopes &, sym::Scope *);
  
  template <typename... Args>
  sym::Scope *enterScope(Args &&... args) {
    return pushScope(std::make_unique<sym::Scope>(scope, std::forward<Args>(args)...));
  }
  
  void leaveScope();
  sym::Scope *cur() const;

private:
  sym::Scopes &scopes;
  sym::Scope *scope;
  
  sym::Scope *pushScope(std::unique_ptr<sym::Scope>);
};

}

#endif
