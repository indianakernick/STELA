//
//  expression builder.cpp
//  STELA
//
//  Created by Indi Kernick on 16/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "expression builder.hpp"

#include "generate expr.hpp"
#include "generate zero expr.hpp"

using namespace stela;

ExprBuilder::ExprBuilder(gen::Ctx ctx, FuncBuilder &fn)
  : ctx{ctx}, fn{fn} {}

llvm::Value *ExprBuilder::zero(ast::Type *type) {
  return generateZeroExpr(ctx, fn, type);
}

llvm::Value *ExprBuilder::addr(ast::Expression *expr) {
  return generateAddrExpr(ctx, fn, expr);
}

llvm::Value *ExprBuilder::value(ast::Expression *expr) {
  return generateValueExpr(ctx, fn, expr);
}

llvm::Value *ExprBuilder::expr(ast::Expression *expr) {
  return generateExpr(ctx, fn, expr);
}

llvm::Value *ExprBuilder::value(llvm::Value *value) {
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
  fn.ir.CreateCondBr(value(cond), troo, folse);
}
