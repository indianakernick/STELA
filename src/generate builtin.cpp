//
//  generate builtin.cpp
//  STELA
//
//  Created by Indi Kernick on 12/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "gen types.hpp"
#include "gen helpers.hpp"
#include "generate type.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

template <>
llvm::Function *stela::genFn<PFGI::btn_capacity>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    lenTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "btn_capacity");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  return array.cap
  */
  
  llvm::Value *array = builder.ir.CreateLoad(func->arg_begin());
  builder.ir.CreateRet(loadStructElem(builder.ir, array, array_idx_cap));
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::btn_size>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    lenTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "btn_size");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  return array.len
  */
  
  llvm::Value *array = builder.ir.CreateLoad(func->arg_begin());
  builder.ir.CreateRet(loadStructElem(builder.ir, array, array_idx_len));
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::btn_pop_back>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "btn_pop_back");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  if array.len != 0
    array.len--
    destroy array.dat[array.len]
    return
  else
    panic
  */
  
  llvm::BasicBlock *popBlock = builder.makeBlock();
  llvm::BasicBlock *panicBlock = builder.makeBlock();
  llvm::Value *array = builder.ir.CreateLoad(func->arg_begin());
  llvm::Value *lenPtr = builder.ir.CreateStructGEP(array, array_idx_len);
  llvm::Value *len = builder.ir.CreateLoad(lenPtr);
  llvm::Value *notEmpty = builder.ir.CreateICmpNE(len, constantFor(len, 0));
  likely(builder.ir.CreateCondBr(notEmpty, popBlock, panicBlock));
  
  builder.setCurr(popBlock);
  llvm::Value *lenMinus1 = builder.ir.CreateNSWSub(len, constantFor(len, 1));
  builder.ir.CreateStore(lenMinus1, lenPtr);
  llvm::Value *dat = loadStructElem(builder.ir, array, array_idx_dat);
  LifetimeExpr lifetime{data.inst, builder.ir};
  lifetime.destroy(arr->elem.get(), arrayIndex(builder.ir, dat, lenMinus1));
  builder.ir.CreateRetVoid();
  
  builder.setCurr(panicBlock);
  callPanic(builder.ir, data.inst.get<FGI::panic>(), "pop_back from empty array");
  
  return func;
}
