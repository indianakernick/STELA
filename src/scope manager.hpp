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
  ScopeMan(sym::Scopes &, sym::Scope *);
  
  sym::Scope *enterScope(sym::Scope::Type);
  void leaveScope();
  sym::Scope *cur() const;
  sym::Scope *builtin() const;
  sym::Scope *global() const;

private:
  sym::Scopes &scopes;
  sym::Scope *scope;
};

}

#endif
