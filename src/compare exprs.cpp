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

llvm::Value *CompareExpr::eq(ast::Type *type, gen::Expr left, gen::Expr right) {
  ast::Type *concrete = concreteType(type);
  if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
    INT_FLOAT_OP(CreateICmpEQ, CreateFCmpOEQ);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::arr_eq>(arr), {left.obj, right.obj});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::clo_eq>(clo), {left.obj, right.obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::srt_eq>(srt), {left.obj, right.obj});
  } else {
    UNREACHABLE();
  }
}

llvm::Value *CompareExpr::ne(ast::Type *type, gen::Expr left, gen::Expr right) {
  return ir.CreateNot(eq(type, left, right));
}

llvm::Value *CompareExpr::lt(ast::Type *type, gen::Expr left, gen::Expr right) {
  ast::Type *concrete = concreteType(type);
  if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
    SIGNED_UNSIGNED_FLOAT_OP(CreateICmpSLT, CreateICmpULT, CreateFCmpOLT);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::arr_lt>(arr), {left.obj, right.obj});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::clo_lt>(clo), {left.obj, right.obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::srt_lt>(srt), {left.obj, right.obj});
  } else {
    UNREACHABLE();
  }
}

llvm::Value *CompareExpr::gt(ast::Type *type, gen::Expr left, gen::Expr right) {
  return lt(type, right, left);
}

llvm::Value *CompareExpr::le(ast::Type *type, gen::Expr left, gen::Expr right) {
  return ir.CreateNot(lt(type, right, left));
}

llvm::Value *CompareExpr::ge(ast::Type *type, gen::Expr left, gen::Expr right) {
  return ir.CreateNot(lt(type, left, right));
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
