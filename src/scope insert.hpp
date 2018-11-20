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
#include "context.hpp"

namespace stela {
 
void insert(sym::Ctx, const sym::Name &, sym::SymbolPtr);
sym::Func *insert(sym::Ctx, ast::Func &);
void enterFuncScope(sym::Func *, ast::Func &);

template <typename Symbol, typename AST_Node>
Symbol *insert(sym::Ctx ctx, AST_Node &node) {
  auto symbol = std::make_unique<Symbol>();
  Symbol *const ret = symbol.get();
  symbol->loc = node.loc;
  symbol->node = {retain, &node};
  insert(ctx, sym::Name{node.name}, std::move(symbol));
  return ret;
}

}

#endif
