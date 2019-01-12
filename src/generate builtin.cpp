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
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

template <>
llvm::Function *stela::genFn<PFGI::capacity>(InstData data, ast::ArrayType *arr) {
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
llvm::Function *stela::genFn<PFGI::size>(InstData data, ast::ArrayType *arr) {
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
