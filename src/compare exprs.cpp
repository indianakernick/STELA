//
//  compare exprs.cpp
//  STELA
//
//  Created by Indi Kernick on 8/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "compare exprs.hpp"

#include "unreachable.hpp"
#include "generate type.hpp"

using namespace stela;

#define INT_FLOAT_OP(I_OP, F_OP)                                                \
  const ArithCat arith = classifyArith(btn);                                    \
  if (arith == ArithCat::floating_point) {                                      \
    return ir.F_OP(getBtnValue(left), getBtnValue(right));                      \
  } else {                                                                      \
    return ir.I_OP(getBtnValue(left), getBtnValue(right));                      \
  }

#define SIGNED_UNSIGNED_FLOAT_OP(S_OP, U_OP, F_OP)                              \
  const ArithCat arith = classifyArith(btn);                                    \
  if (arith == ArithCat::signed_int) {                                          \
    return ir.S_OP(getBtnValue(left), getBtnValue(right));                      \
  } else if (arith == ArithCat::unsigned_int) {                                 \
    return ir.U_OP(getBtnValue(left), getBtnValue(right));                      \
  } else {                                                                      \
    return ir.F_OP(getBtnValue(left), getBtnValue(right));                      \
  }

CompareExpr::CompareExpr(FuncInst &inst, llvm::IRBuilder<> &ir)
  : inst{inst}, ir{ir} {}

llvm::Value *CompareExpr::equal(ast::Type *type, gen::Expr left, gen::Expr right) {
  ast::Type *concrete = concreteType(type);
  if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
    INT_FLOAT_OP(CreateICmpEQ, CreateFCmpOEQ);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    return ir.CreateCall(inst.arrayEq(arr), {left.obj, right.obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    return ir.CreateCall(inst.structEq(srt), {left.obj, right.obj});
  } else {
    UNREACHABLE();
  }
}

llvm::Value *CompareExpr::notEqual(ast::Type *type, gen::Expr left, gen::Expr right) {
  return ir.CreateNot(equal(type, left, right));
}

llvm::Value *CompareExpr::less(ast::Type *type, gen::Expr left, gen::Expr right) {
  ast::Type *concrete = concreteType(type);
  if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
    SIGNED_UNSIGNED_FLOAT_OP(CreateICmpSLT, CreateICmpULT, CreateFCmpOLT);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    return ir.CreateCall(inst.arrayLt(arr), {left.obj, right.obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    return ir.CreateCall(inst.structLt(srt), {left.obj, right.obj});
  } else {
    UNREACHABLE();
  }
}

llvm::Value *CompareExpr::greater(ast::Type *type, gen::Expr left, gen::Expr right) {
  return less(type, right, left);
}

llvm::Value *CompareExpr::lessEqual(ast::Type *type, gen::Expr left, gen::Expr right) {
  return ir.CreateNot(less(type, right, left));
}

llvm::Value *CompareExpr::greaterEqual(ast::Type *type, gen::Expr left, gen::Expr right) {
  return ir.CreateNot(less(type, left, right));
}

llvm::Value *CompareExpr::getBtnValue(gen::Expr expr) {
  if (expr.cat == ValueCat::prvalue) {
    return expr.obj;
  } else {
    return ir.CreateLoad(expr.obj);
  }
}

#undef SIGNED_UNSIGNED_FLOAT_OP
#undef INT_FLOAT_OP
