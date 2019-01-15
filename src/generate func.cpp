//
//  generate func.cpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "inst data.hpp"
#include "gen types.hpp"
#include "gen helpers.hpp"
#include "compare exprs.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

template <>
llvm::Function *stela::genFn<FGI::panic>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::Type *strTy = getType<char>(ctx)->getPointerTo();
  llvm::Type *intTy = getType<int>(ctx);
  llvm::FunctionType *putsType = llvm::FunctionType::get(intTy, {strTy}, false);
  llvm::FunctionType *exitType = llvm::FunctionType::get(voidTy(ctx), {intTy}, false);
  llvm::FunctionType *panicType = llvm::FunctionType::get(voidTy(ctx), {strTy}, false);
  
  llvm::Function *puts = declareCFunc(data.mod, putsType, "puts");
  llvm::Function *exit = declareCFunc(data.mod, exitType, "exit");
  llvm::Function *panic = makeInternalFunc(data.mod, panicType, "panic", Inline::never);
  puts->addParamAttr(0, llvm::Attribute::ReadOnly);
  puts->addParamAttr(0, llvm::Attribute::NoCapture);
  exit->addFnAttr(llvm::Attribute::NoReturn);
  exit->addFnAttr(llvm::Attribute::Cold);
  panic->addFnAttr(llvm::Attribute::NoReturn);
  panic->addFnAttr(llvm::Attribute::Cold);
  
  FuncBuilder builder{panic};
  builder.ir.CreateCall(puts, panic->arg_begin());
  builder.ir.CreateCall(exit, constantFor(intTy, EXIT_FAILURE));
  builder.ir.CreateUnreachable();
  
  return panic;
}

template <>
llvm::Function *stela::genFn<FGI::alloc>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::Type *memTy = voidPtrTy(ctx);
  llvm::Type *sizeTy = getType<size_t>(ctx);
  llvm::FunctionType *mallocType = llvm::FunctionType::get(memTy, {sizeTy}, false);
  llvm::FunctionType *allocType = mallocType;
  
  llvm::Function *malloc = declareCFunc(data.mod, mallocType, "malloc");
  llvm::Function *alloc = makeInternalFunc(data.mod, allocType, "alloc");
  malloc->addAttribute(0, llvm::Attribute::NoAlias);
  alloc->addAttribute(0, llvm::Attribute::NoAlias);
  alloc->addAttribute(0, llvm::Attribute::NonNull);
  FuncBuilder builder{alloc};
  
  llvm::BasicBlock *okBlock = builder.makeBlock();
  llvm::BasicBlock *errorBlock = builder.makeBlock();
  llvm::Value *ptr = builder.ir.CreateCall(malloc, alloc->arg_begin());
  llvm::Value *isNotNull = builder.ir.CreateIsNotNull(ptr);
  likely(builder.ir.CreateCondBr(isNotNull, okBlock, errorBlock));
  
  builder.setCurr(okBlock);
  builder.ir.CreateRet(ptr);
  builder.setCurr(errorBlock);
  callPanic(builder.ir, data.inst.get<FGI::panic>(), "Out of memory");
  
  return alloc;
}

template <>
llvm::Function *stela::genFn<FGI::free>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *memTy = voidPtrTy(ctx);
  llvm::FunctionType *freeType = llvm::FunctionType::get(voidTy(ctx), {memTy}, false);
  llvm::Function *free = declareCFunc(data.mod, freeType, "free");
  free->addParamAttr(0, llvm::Attribute::NoCapture);
  return free;
}

template <>
llvm::Function *stela::genFn<FGI::ceil_to_pow_2>(InstData data) {
  // this returns 2 when the input is 1
  // we could subtract val == 1 but that's just more instructions for no reason!
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = lenTy(ctx);
  llvm::FunctionType *fnType = llvm::FunctionType::get(
    type, {type}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, fnType, "ceil_to_pow_2");
  FuncBuilder builder{func};
  
  llvm::DataLayout layout{data.mod};
  const uint64_t bitSize = layout.getTypeSizeInBits(type);
  llvm::FunctionType *ctlzType = llvm::FunctionType::get(
    type, {type, builder.ir.getInt1Ty()}, false
  );
  llvm::Twine name = llvm::Twine{"llvm.ctlz.i"} + llvm::Twine{bitSize};
  llvm::Function *ctlz = declareCFunc(data.mod, ctlzType, name);
  llvm::Value *minus1 = builder.ir.CreateNSWSub(func->arg_begin(), constantFor(type, 1));
  llvm::Value *leadingZeros = builder.ir.CreateCall(ctlz, {minus1, builder.ir.getTrue()});
  llvm::Value *bits = constantFor(type, bitSize);
  llvm::Value *log2 = builder.ir.CreateNSWSub(bits, leadingZeros);
  llvm::Value *ceiled = builder.ir.CreateShl(constantFor(type, 1), log2);
  
  builder.ir.CreateRet(ceiled);
  return func;
}
