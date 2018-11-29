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
#include "context.hpp"

namespace stela {

bool boolOp(ast::BinOp);
bool boolOp(ast::UnOp);

bool validIncr(const ast::BtnTypePtr &);
bool validOp(ast::UnOp, const ast::BtnTypePtr &);
retain_ptr<ast::BtnType> validOp(
  const sym::Builtins &,
  ast::BinOp,
  const ast::BtnTypePtr &,
  const ast::BtnTypePtr &
);
bool validOp(ast::AssignOp, const ast::BtnTypePtr &);
bool validSubscript(const ast::BtnTypePtr &);

ast::TypePtr callBtnFunc(sym::Ctx, ast::BtnFuncEnum, const sym::FuncParams &, Loc);

sym::ScopePtr makeBuiltinModule(sym::Builtins &);

}

#endif
