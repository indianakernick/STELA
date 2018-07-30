//
//  scope insert.hpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_insert_hpp
#define stela_scope_insert_hpp

#include "symbols.hpp"
#include "log output.hpp"

namespace stela {

void insert(sym::Scope *, Log &, sym::Name, sym::SymbolPtr);
sym::Func *insert(sym::Scope *, Log &, sym::Name, sym::FuncPtr);

template <typename Symbol>
Symbol *insert(sym::Scope *scope, Log &log, const sym::Name name) {
  auto symbol = std::make_unique<Symbol>();
  Symbol *const ret = symbol.get();
  insert(scope, log, name, std::move(symbol));
  return ret;
}
template <typename Symbol, typename AST_Node>
Symbol *insert(sym::Scope *scope, Log &log, const AST_Node &node) {
  Symbol *const symbol = insert<Symbol>(scope, log, sym::Name(node.name));
  symbol->loc = node.loc;
  return symbol;
}

void insert(sym::StructScope *, Log &, const sym::MemKey &, sym::SymbolPtr);
sym::Func *insert(sym::StructScope *, Log &, const sym::MemFunKey &, sym::FuncPtr);

}

#endif
