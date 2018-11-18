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

bool validIncr(const ast::BtnTypePtr &);
bool validOp(ast::UnOp, const ast::BtnTypePtr &);
retain_ptr<ast::BtnType> validOp(
  const sym::Builtins &,
  ast::BinOp,
  const ast::BtnTypePtr &,
  const ast::BtnTypePtr &
);
bool validOp(ast::AssignOp, const ast::BtnTypePtr &, const ast::BtnTypePtr &);
bool validSubscript(const ast::BtnTypePtr &);

sym::Module makeBuiltinModule(sym::Builtins &);

}

#endif
