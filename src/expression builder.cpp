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

gen::Expr ExprBuilder::value(ast::Expression *expr) {
  return generateValueExpr(ctx, fn, expr);
}

gen::Expr ExprBuilder::expr(ast::Expression *expr, llvm::Value *result) {
  return generateExpr(ctx, fn, expr, result);
}

void ExprBuilder::condBr(
  ast::Expression *cond,
  llvm::BasicBlock *troo,
  llvm::BasicBlock *folse
) {
  fn.ir.CreateCondBr(value(cond).obj, troo, folse);
}
