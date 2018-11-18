//
//  builtin symbols.hpp
//  STELA
//
//  Created by Indi Kernick on 16/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_builtin_symbols_hpp
#define stela_builtin_symbols_hpp

#include "ast.hpp"
#include "symbols.hpp"

namespace stela {

bool validIncr(const ast::BuiltinTypePtr &);
bool validOp(ast::UnOp, const ast::BuiltinTypePtr &);
retain_ptr<ast::BuiltinType> validOp(
  const sym::Builtins &,
  ast::BinOp,
  const ast::BuiltinTypePtr &,
  const ast::BuiltinTypePtr &
);
bool validOp(ast::AssignOp, const ast::BuiltinTypePtr &, const ast::BuiltinTypePtr &);
bool validSubscript(const ast::BuiltinTypePtr &);

sym::Module makeBuiltinModule(sym::Builtins &);

}

#endif
