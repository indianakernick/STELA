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
  InserterManager(sym::Scope *, Log &);
  
  template <typename Symbol, typename AST_Node>
  Symbol *insert(const AST_Node &node) {
    auto symbol = std::make_unique<Symbol>();
    Symbol *const ret = symbol.get();
    symbol->loc = node.loc;
    symbol->node = const_cast<AST_Node *>(&node);
    insert(sym::Name(node.name), std::move(symbol));
    return ret;
  }
  
  void insert(const sym::Name &, sym::SymbolPtr);
  sym::Func *insert(const ast::Func &);
  void enterFuncScope(sym::Func *, const ast::Func &);
  
  void push(sym::Scope *);
  void pop();
  sym::Scope *scope() const;

private:
  std::vector<sym::Scope *> stack;
  Log &log;
  NameLookup lkp;
};

}

#endif
