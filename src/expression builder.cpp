//
//  expression builder.cpp
//  STELA
//
//  Created by Indi Kernick on 16/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "expression builder.hpp"

using namespace stela;

ExprBuilder::ExprBuilder(gen::Ctx ctx, FuncBuilder &fn)
  : ctx{ctx}, fn{fn} {}

gen::Expr ExprBuilder::value(Scope &temps, ast::Expression *expr) {
  return generateValueExpr(temps, ctx, fn, expr);
}

void ExprBuilder::expr(Scope &temps, ast::Expression *expr, llvm::Value *result) {
  generateExpr(temps, ctx, fn, expr, result);
}

gen::Expr ExprBuilder::expr(Scope &temps, ast::Expression *expr) {
  return generateExpr(temps, ctx, fn, expr, nullptr);
}

void ExprBuilder::condBr(
  Scope &temps,
  ast::Expression *cond,
  llvm::BasicBlock *troo,
  llvm::BasicBlock *fols
) {
  fn.ir.CreateCondBr(value(temps, cond).obj, troo, fols);
}
