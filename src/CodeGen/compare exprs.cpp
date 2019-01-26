//
//  compare exprs.cpp
//  STELA
//
//  Created by Indi Kernick on 8/1/19.
//  Copyrhs © 2019 Indi Kernick. All rhss reserved.
//

#include "compare exprs.hpp"

#include "generate type.hpp"
#include "Utils/unreachable.hpp"

using namespace stela;

#define INT_FLOAT_OP(I_OP, F_OP)                                                \
  const ArithCat arith = classifyArith(btn);                                    \
  if (arith == ArithCat::floating_point) {                                      \
    return ir.F_OP(getBtnValue(lhs), getBtnValue(rhs));                         \
  } else {                                                                      \
    return ir.I_OP(getBtnValue(lhs), getBtnValue(rhs));                         \
  }

#define SIGNED_UNSIGNED_FLOAT_OP(S_OP, U_OP, F_OP)                              \
  const ArithCat arith = classifyArith(btn);                                    \
  if (arith == ArithCat::signed_int) {                                          \
    return ir.S_OP(getBtnValue(lhs), getBtnValue(rhs));                         \
  } else if (arith == ArithCat::unsigned_int) {                                 \
    return ir.U_OP(getBtnValue(lhs), getBtnValue(rhs));                         \
  } else {                                                                      \
    return ir.F_OP(getBtnValue(lhs), getBtnValue(rhs));                         \
  }

CompareExpr::CompareExpr(FuncInst &inst, llvm::IRBuilder<> &ir)
  : inst{inst}, ir{ir} {}

llvm::Value *CompareExpr::eq(ast::Type *type, gen::Expr lhs, gen::Expr rhs) {
  ast::Type *concrete = concreteType(type);
  if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
    INT_FLOAT_OP(CreateICmpEQ, CreateFCmpOEQ);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::arr_eq>(arr), {lhs.obj, rhs.obj});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::clo_eq>(clo), {lhs.obj, rhs.obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::srt_eq>(srt), {lhs.obj, rhs.obj});
  } else {
    UNREACHABLE();
  }
}

llvm::Value *CompareExpr::ne(ast::Type *type, gen::Expr lhs, gen::Expr rhs) {
  return ir.CreateNot(eq(type, lhs, rhs));
}

llvm::Value *CompareExpr::lt(ast::Type *type, gen::Expr lhs, gen::Expr rhs) {
  ast::Type *concrete = concreteType(type);
  if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
    SIGNED_UNSIGNED_FLOAT_OP(CreateICmpSLT, CreateICmpULT, CreateFCmpOLT);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::arr_lt>(arr), {lhs.obj, rhs.obj});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::clo_lt>(clo), {lhs.obj, rhs.obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    return ir.CreateCall(inst.get<PFGI::srt_lt>(srt), {lhs.obj, rhs.obj});
  } else {
    UNREACHABLE();
  }
}

llvm::Value *CompareExpr::gt(ast::Type *type, gen::Expr lhs, gen::Expr rhs) {
  return lt(type, rhs, lhs);
}

llvm::Value *CompareExpr::le(ast::Type *type, gen::Expr lhs, gen::Expr rhs) {
  return ir.CreateNot(lt(type, rhs, lhs));
}

llvm::Value *CompareExpr::ge(ast::Type *type, gen::Expr lhs, gen::Expr rhs) {
  return ir.CreateNot(lt(type, lhs, rhs));
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
