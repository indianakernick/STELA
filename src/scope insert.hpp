//
//  scope insert.hpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_insert_hpp
#define stela_scope_insert_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "log output.hpp"
#include "scope lookup.hpp"

namespace stela {

class InserterManager {
public:
  InserterManager(sym::Modules &, ScopeMan &, Log &);
  
  template <typename Symbol, typename AST_Node>
  Symbol *insert(AST_Node &node) {
    auto symbol = std::make_unique<Symbol>();
    Symbol *const ret = symbol.get();
    symbol->loc = node.loc;
    symbol->node = {retain, &node};
    insert(sym::Name(node.name), std::move(symbol));
    return ret;
  }
  
  void insert(const sym::Name &, sym::SymbolPtr);
  sym::Func *insert(ast::Func &);
  void enterFuncScope(sym::Func *, ast::Func &);

private:
  Log &log;
  ScopeMan &man;
  NameLookup tlk;
};

}

#endif
