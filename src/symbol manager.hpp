//
//  symbol manager.hpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_symbol_manager_hpp
#define stela_symbol_manager_hpp

#include <string>
#include "ast.hpp"
#include "symbols.hpp"
#include "log output.hpp"

namespace stela {

class SymbolMan {
public:
  SymbolMan(sym::Scopes &, Log &);
  
  std::string typeName(const ast::TypePtr &);
  sym::FuncParams funcParams(const ast::FuncParams &);
  
  void enterScope();
  void leaveScope();
  
  sym::Symbol *lookup(sym::Name, Loc);
  sym::Func *lookup(sym::Name, const sym::FuncParams &, Loc);
  
  void insert(sym::Name, sym::SymbolPtr);
  void insert(sym::Name, sym::FuncPtr);

private:
  sym::Scopes &scopes;
  sym::Scope *scope;
  Log &log;
};

}

#endif
