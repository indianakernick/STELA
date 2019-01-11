//
//  generate struct.cpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "generate struct.hpp"

#include "gen helpers.hpp"
#include "generate type.hpp"
#include "compare exprs.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"

using namespace stela;

namespace {

llvm::Function *unarySrt(
  InstData data,
  ast::StructType *srt,
  const llvm::Twine &name,
  void (LifetimeExpr::*memFun)(ast::Type *, llvm::Value *)
) {
  llvm::Type *type = generateType(data.mod->getContext(), srt);
  llvm::Function *func = makeInternalFunc(data.mod, unaryCtorFor(type), name);
  assignUnaryCtorAttrs(func);
  FuncBuilder funcBdr{func};
  LifetimeExpr lifetime{data.inst, funcBdr.ir};
  
  const unsigned members = type->getNumContainedTypes();
  for (unsigned m = 0; m != members; ++m) {
    llvm::Value *memPtr = funcBdr.ir.CreateStructGEP(func->arg_begin(), m);
    (lifetime.*memFun)(srt->fields[m].type.get(), memPtr);
  }
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *binarySrt(
  InstData data,
  ast::StructType *srt,
  const llvm::Twine &name,
  void (LifetimeExpr::*memFun)(ast::Type *, llvm::Value *, llvm::Value *)
) {
  llvm::Type *type = generateType(data.mod->getContext(), srt);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), name);
  if (memFun == &LifetimeExpr::copyAssign) {
    assignBinaryAliasCtorAttrs(func);
  } else {
    assignBinaryCtorAttrs(func);
  }
  FuncBuilder funcBdr{func};
  LifetimeExpr lifetime{data.inst, funcBdr.ir};
  
  const unsigned members = type->getNumContainedTypes();
  for (unsigned m = 0; m != members; ++m) {
    llvm::Value *dstPtr = funcBdr.ir.CreateStructGEP(func->arg_begin(), m);
    llvm::Value *srcPtr = funcBdr.ir.CreateStructGEP(func->arg_begin() + 1, m);
    (lifetime.*memFun)(srt->fields[m].type.get(), dstPtr, srcPtr);
  }
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

}

llvm::Function *stela::genSrtDtor(InstData data, ast::StructType *srt) {
  return unarySrt(data, srt, "srt_dtor", &LifetimeExpr::destroy);
}

llvm::Function *stela::genSrtDefCtor(InstData data, ast::StructType *srt) {
  return unarySrt(data, srt, "srt_def_ctor", &LifetimeExpr::defConstruct);
}

llvm::Function *stela::genSrtCopCtor(InstData data, ast::StructType *srt) {
  return binarySrt(data, srt, "srt_cop_ctor", &LifetimeExpr::copyConstruct);
}

llvm::Function *stela::genSrtCopAsgn(InstData data, ast::StructType *srt) {
  return binarySrt(data, srt, "srt_cop_asgn", &LifetimeExpr::copyAssign);
}

llvm::Function *stela::genSrtMovCtor(InstData data, ast::StructType *srt) {
  return binarySrt(data, srt, "srt_mov_ctor", &LifetimeExpr::moveConstruct);
}

llvm::Function *stela::genSrtMovAsgn(InstData data, ast::StructType *srt) {
  return binarySrt(data, srt, "srt_mov_asgn", &LifetimeExpr::moveAssign);
}

llvm::Function *stela::genSrtEq(InstData data, ast::StructType *srt) {
  llvm::Type *type = generateType(data.mod->getContext(), srt);
  llvm::Function *func = makeInternalFunc(data.mod, compareFor(type), "srt_eq");
  assignCompareAttrs(func);
  FuncBuilder funcBdr{func};
  CompareExpr compare{data.inst, funcBdr.ir};
  llvm::BasicBlock *diffBlock = funcBdr.makeBlock();
  
  /*
  for m in struct
    if left.m == right.m
      continue
    else
      return false
  return true
  */
  
  const unsigned members = type->getNumContainedTypes();
  for (unsigned m = 0; m != members; ++m) {
    llvm::Value *lPtr = funcBdr.ir.CreateStructGEP(func->arg_begin(), m);
    llvm::Value *rPtr = funcBdr.ir.CreateStructGEP(func->arg_begin() + 1, m);
    ast::Type *field = srt->fields[m].type.get();
    llvm::Value *eq = compare.equal(field, lvalue(lPtr), lvalue(rPtr));
    llvm::BasicBlock *equalBlock = funcBdr.makeBlock();
    funcBdr.ir.CreateCondBr(eq, equalBlock, diffBlock);
    funcBdr.setCurr(equalBlock);
  }
  
  returnBool(funcBdr.ir, true);
  funcBdr.setCurr(diffBlock);
  returnBool(funcBdr.ir, false);
  return func;
}

llvm::Function *stela::genSrtLt(InstData data, ast::StructType *srt) {
  llvm::Type *type = generateType(data.mod->getContext(), srt);
  llvm::Function *func = makeInternalFunc(data.mod, compareFor(type), "srt_lt");
  assignCompareAttrs(func);
  FuncBuilder funcBdr{func};
  CompareExpr compare{data.inst, funcBdr.ir};
  llvm::BasicBlock *ltBlock = funcBdr.makeBlock();
  llvm::BasicBlock *geBlock = funcBdr.makeBlock();
  
  /*
  for m in struct
    if left.m < right.m
      return true
    else
      if right.m < left.m
        return false
      else
        continue
  return false
  */
  
  const unsigned members = type->getNumContainedTypes();
  for (unsigned m = 0; m != members; ++m) {
    llvm::Value *lPtr = funcBdr.ir.CreateStructGEP(func->arg_begin(), m);
    llvm::Value *rPtr = funcBdr.ir.CreateStructGEP(func->arg_begin() + 1, m);
    ast::Type *field = srt->fields[m].type.get();
    llvm::Value *less = compare.less(field, lvalue(lPtr), lvalue(rPtr));
    llvm::BasicBlock *notLessBlock = funcBdr.makeBlock();
    funcBdr.ir.CreateCondBr(less, ltBlock, notLessBlock);
    funcBdr.setCurr(notLessBlock);
    llvm::Value *greater = compare.less(field, lvalue(rPtr), lvalue(lPtr));
    llvm::BasicBlock *equalBlock = funcBdr.makeBlock();
    funcBdr.ir.CreateCondBr(greater, geBlock, equalBlock);
    funcBdr.setCurr(equalBlock);
  }
  
  funcBdr.ir.CreateBr(geBlock);
  funcBdr.setCurr(ltBlock);
  returnBool(funcBdr.ir, true);
  funcBdr.setCurr(geBlock);
  returnBool(funcBdr.ir, false);
  return func;
}
