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

gen::Expr ExprBuilder::addr(ast::Expression *expr) {
  return generateAddrExpr(ctx, fn, expr);
}

gen::Expr ExprBuilder::value(ast::Expression *expr) {
  return generateValueExpr(ctx, fn, expr);
}

gen::Expr ExprBuilder::expr(ast::Expression *expr) {
  return generateExpr(ctx, fn, expr);
}

llvm::Value *ExprBuilder::addr(llvm::Value *value) {
  // @FIXME remove or use value categories
  if (value->getType()->isPointerTy()) {
    return value;
  } else {
    return fn.allocStore(value);
  }
}

llvm::Value *ExprBuilder::value(llvm::Value *value) {
  // @FIXME remove or use value categories
  if (value->getType()->isPointerTy()) {
    return fn.ir.CreateLoad(value);
  } else {
    return value;
  }
}

void ExprBuilder::condBr(
  ast::Expression *cond,
  llvm::BasicBlock *troo,
  llvm::BasicBlock *folse
) {
  fn.ir.CreateCondBr(value(cond).obj, troo, folse);
}
