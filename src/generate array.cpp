//
//  generate array.cpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "inst data.hpp"
#include "gen helpers.hpp"
#include "generate type.hpp"
#include "compare exprs.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"

using namespace stela;

template <>
llvm::Function *stela::genFn<PFGI::arr_dtor>(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, unaryCtorFor(type), "arr_dtor");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  ptr_dtor(arr_strg_dtor, bitcast obj)
  */
  
  llvm::Function *strgDtor = data.inst.get<PFGI::arr_strg_dtor>(arr);
  llvm::Value *obj = refPtrPtrCast(builder.ir, func->arg_begin());
  builder.ir.CreateCall(data.inst.get<FGI::ptr_dtor>(), {strgDtor, obj});
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::arr_def_ctor>(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, unaryCtorFor(type), "arr_def_ctor");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  ptr_def_ctor(bitcast obj, sizeof storage)
  obj.cap = 0
  obj.len = 0
  obj.dat = null
  */
  
  llvm::Value *arrayPtr = func->arg_begin();
  llvm::Type *storageTy = type->getPointerElementType();
  llvm::Value *array = callAlloc(builder.ir, data.inst.get<FGI::alloc>(), storageTy);
  initRefCount(builder.ir, array);

  llvm::Value *cap = builder.ir.CreateStructGEP(array, array_idx_cap);
  builder.ir.CreateStore(constantForPtr(cap, 0), cap);
  llvm::Value *len = builder.ir.CreateStructGEP(array, array_idx_len);
  builder.ir.CreateStore(constantForPtr(len, 0), len);
  llvm::Value *dat = builder.ir.CreateStructGEP(array, array_idx_dat);
  setNull(builder.ir, dat);
  builder.ir.CreateStore(array, arrayPtr);
  
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::arr_cop_ctor>(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), "arr_cop_ctor");
  assignBinaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  ptr_cop_ctor(bitcast obj, bitcast other)
  */
  
  llvm::Value *objPtr = refPtrPtrCast(builder.ir, func->arg_begin());
  llvm::Value *otherPtr = refPtrPtrCast(builder.ir, func->arg_begin() + 1);
  builder.ir.CreateCall(data.inst.get<FGI::ptr_cop_ctor>(), {objPtr, otherPtr});
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::arr_cop_asgn>(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), "arr_cop_asgn");
  assignBinaryAliasCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  ptr_cop_asgn(arr_strg_dtor, bitcast left, bitcast right)
  */
  
  llvm::Function *strgDtor = data.inst.get<PFGI::arr_strg_dtor>(arr);
  llvm::Value *leftPtr = refPtrPtrCast(builder.ir, func->arg_begin());
  llvm::Value *rightPtr = refPtrPtrCast(builder.ir, func->arg_begin() + 1);
  builder.ir.CreateCall(data.inst.get<FGI::ptr_cop_asgn>(), {strgDtor, leftPtr, rightPtr});
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::arr_mov_ctor>(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), "arr_mov_ctor");
  assignBinaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  ptr_mov_ctor(bitcast obj, bitcast other)
  */
  
  llvm::Value *objPtr = refPtrPtrCast(builder.ir, func->arg_begin());
  llvm::Value *otherPtr = refPtrPtrCast(builder.ir, func->arg_begin() + 1);
  builder.ir.CreateCall(data.inst.get<FGI::ptr_mov_ctor>(), {objPtr, otherPtr});
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::arr_mov_asgn>(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), "arr_mov_asgn");
  assignBinaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  ptr_mov_asgn(arr_strg_dtor, bitcast left, bitcast right)
  */
  
  llvm::Function *strgDtor = data.inst.get<PFGI::arr_strg_dtor>(arr);
  llvm::Value *leftPtr = refPtrPtrCast(builder.ir, func->arg_begin());
  llvm::Value *rightPtr = refPtrPtrCast(builder.ir, func->arg_begin() + 1);
  builder.ir.CreateCall(data.inst.get<FGI::ptr_mov_asgn>(), {strgDtor, leftPtr, rightPtr});
  builder.ir.CreateRetVoid();
  return func;
}

namespace {

using BoundsChecker = llvm::Value *(llvm::IRBuilder<> &, llvm::Value *, llvm::Value *);

llvm::Function *generateArrayIdx(
  InstData data,
  ast::ArrayType *arr,
  BoundsChecker *checkBounds,
  const llvm::Twine &name
) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Type *arrayStruct = type->getPointerElementType();
  llvm::Type *elemPtr = arrayStruct->getStructElementType(array_idx_dat);
  llvm::Type *lenType = arrayStruct->getStructElementType(array_idx_len);
  llvm::FunctionType *fnType = llvm::FunctionType::get(
    elemPtr,
    {type->getPointerTo(), lenType},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, fnType, name);
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  if checkBounds(idx, array.len)
    return array.dat[idx]
  else
    panic
  */
  
  llvm::BasicBlock *okBlock = builder.makeBlock();
  llvm::BasicBlock *errorBlock = builder.makeBlock();
  llvm::Value *array = builder.ir.CreateLoad(func->arg_begin());
  llvm::Value *len = loadStructElem(builder.ir, array, array_idx_len);
  llvm::Value *inBounds = checkBounds(builder.ir, func->arg_begin() + 1, len);
  likely(builder.ir.CreateCondBr(inBounds, okBlock, errorBlock));
  
  builder.setCurr(okBlock);
  llvm::Value *dat = loadStructElem(builder.ir, array, array_idx_dat);
  builder.ir.CreateRet(arrayIndex(builder.ir, dat, func->arg_begin() + 1));
  
  builder.setCurr(errorBlock);
  callPanic(builder.ir, data.inst.get<FGI::panic>(), "Index out of bounds");
  
  return func;
}

llvm::Value *checkSignedBounds(llvm::IRBuilder<> &ir, llvm::Value *index, llvm::Value *size) {
  llvm::Value *aboveZero = ir.CreateICmpSGE(index, constantFor(index, 0));
  llvm::Value *belowSize = ir.CreateICmpSLT(index, size);
  return ir.CreateAnd(aboveZero, belowSize);
}

llvm::Value *checkUnsignedBounds(llvm::IRBuilder<> &ir, llvm::Value *index, llvm::Value *size) {
  return ir.CreateICmpULT(index, size);
}

}

template <>
llvm::Function *stela::genFn<PFGI::arr_idx_s>(InstData data, ast::ArrayType *arr) {
  return generateArrayIdx(data, arr, checkSignedBounds, "arr_idx_s");
}

template <>
llvm::Function *stela::genFn<PFGI::arr_idx_u>(InstData data, ast::ArrayType *arr) {
  return generateArrayIdx(data, arr, checkUnsignedBounds, "arr_idx_u");
}

template <>
llvm::Function *stela::genFn<PFGI::arr_len_ctor>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::Type *arrayStructType = type->getPointerElementType();
  llvm::Type *elemPtr = arrayStructType->getStructElementType(array_idx_dat);
  llvm::Type *elem = elemPtr->getPointerElementType();
  llvm::FunctionType *sig = llvm::FunctionType::get(
    elemPtr,
    {type->getPointerTo(), arrayStructType->getStructElementType(array_idx_len)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "arr_len_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  array = malloc
  array.cap = size
  array.len = size
  array.dat = malloc(size)
  */
  
  llvm::Value *arrayPtr = func->arg_begin();
  llvm::Value *size = func->arg_begin() + 1;
  llvm::Type *storageTy = type->getPointerElementType();
  llvm::Value *array = callAlloc(builder.ir, data.inst.get<FGI::alloc>(), storageTy);
  initRefCount(builder.ir, array);
  
  llvm::Value *cap = builder.ir.CreateStructGEP(array, array_idx_cap);
  builder.ir.CreateStore(size, cap);
  llvm::Value *len = builder.ir.CreateStructGEP(array, array_idx_len);
  builder.ir.CreateStore(size, len);
  llvm::Value *dat = builder.ir.CreateStructGEP(array, array_idx_dat);
  llvm::Value *allocation = callAlloc(builder.ir, data.inst.get<FGI::alloc>(), elem, size);
  builder.ir.CreateStore(allocation, dat);
  builder.ir.CreateStore(array, arrayPtr);
  
  builder.ir.CreateRet(allocation);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::arr_strg_dtor>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = refPtrDtorTy(ctx);
  llvm::Function *func = makeInternalFunc(data.mod, sig, "arr_strg_dtor");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  llvm::Value *storage = builder.ir.CreatePointerCast(func->arg_begin(), type);
  llvm::Value *len = loadStructElem(builder.ir, storage, array_idx_len);
  llvm::Value *dat = loadStructElem(builder.ir, storage, array_idx_dat);
  llvm::Value *destroy_n = data.inst.get<PFGI::destroy_n>(arr->elem.get());
  builder.ir.CreateCall(destroy_n, {dat, len});
  
  builder.ir.CreateRetVoid();
  return func;
}

namespace {

struct ArrPairLoop {
  llvm::BasicBlock *loop;
  llvm::Value *left;
  llvm::Value *right;
  llvm::Value *leftEnd;
  llvm::Value *rightEnd;
};

ArrPairLoop iterArrPair(
  FuncBuilder &builder,
  llvm::Value *leftArr,
  llvm::Value *rightArr,
  llvm::BasicBlock *done
) {
  llvm::BasicBlock *head = builder.makeBlock();
  llvm::BasicBlock *body = builder.makeBlock();
  llvm::BasicBlock *tail = builder.makeBlock();
  
  llvm::Value *leftDat = loadStructElem(builder.ir, leftArr, array_idx_dat);
  llvm::Value *rightDat = loadStructElem(builder.ir, rightArr, array_idx_dat);
  llvm::Value *leftLen = loadStructElem(builder.ir, leftArr, array_idx_len);
  llvm::Value *rightLen = loadStructElem(builder.ir, rightArr, array_idx_len);
  llvm::Value *leftEnd = arrayIndex(builder.ir, leftDat, leftLen);
  llvm::Value *rightEnd = arrayIndex(builder.ir, rightDat, rightLen);
  llvm::Value *leftPtr = builder.allocStore(leftDat);
  llvm::Value *rightPtr = builder.allocStore(rightDat);
  
  builder.branch(head);
  llvm::Value *left = builder.ir.CreateLoad(leftPtr);
  llvm::Value *right = builder.ir.CreateLoad(rightPtr);
  llvm::Value *rightAtEnd = builder.ir.CreateICmpEQ(right, rightEnd);
  builder.ir.CreateCondBr(rightAtEnd, done, body);
  
  builder.setCurr(tail);
  llvm::Value *leftNext = builder.ir.CreateConstInBoundsGEP1_64(left, 1);
  llvm::Value *rightNext = builder.ir.CreateConstInBoundsGEP1_64(right, 1);
  builder.ir.CreateStore(leftNext, leftPtr);
  builder.ir.CreateStore(rightNext, rightPtr);
  builder.ir.CreateBr(head);
  
  builder.setCurr(body);
  return {tail, left, right, leftEnd, rightEnd};
}

}

template <>
llvm::Function *stela::genFn<PFGI::arr_eq>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::Function *func = makeInternalFunc(data.mod, compareFor(type), "arr_eq", Inline::hint);
  assignCompareAttrs(func);
  FuncBuilder builder{func};
  
  /*
  if leftArr.len == rightArr.len
    for left, right in leftArr, rightArr
      if left == right
        continue
      else
        return false
    return true
  else
    return false
  */

  llvm::BasicBlock *compareBlock = builder.makeBlock();
  llvm::BasicBlock *equalBlock = builder.makeBlock();
  llvm::BasicBlock *diffBlock = builder.makeBlock();
  llvm::Value *leftArr = builder.ir.CreateLoad(func->arg_begin());
  llvm::Value *rightArr = builder.ir.CreateLoad(func->arg_begin() + 1);
  llvm::Value *leftLen = loadStructElem(builder.ir, leftArr, array_idx_len);
  llvm::Value *rightLen = loadStructElem(builder.ir, rightArr, array_idx_len);
  llvm::Value *sameLen = builder.ir.CreateICmpEQ(leftLen, rightLen);
  builder.ir.CreateCondBr(sameLen, compareBlock, diffBlock);
  
  builder.setCurr(compareBlock);
  ArrPairLoop comp = iterArrPair(builder, leftArr, rightArr, equalBlock);
  CompareExpr compare{data.inst, builder.ir};
  llvm::Value *eq = compare.equal(arr->elem.get(), lvalue(comp.left), lvalue(comp.right));
  builder.ir.CreateCondBr(eq, comp.loop, diffBlock);
  
  builder.setCurr(equalBlock);
  returnBool(builder.ir, true);
  builder.setCurr(diffBlock);
  returnBool(builder.ir, false);
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::arr_lt>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::Function *func = makeInternalFunc(data.mod, compareFor(type), "arr_lt", Inline::hint);
  assignCompareAttrs(func);
  FuncBuilder builder{func};
  
  /*
  for left, right in leftArr, rightArr
    if left == leftEnd
      return true
    else if left < right
      return true
    else if right < left
      return false
    else
      continue
  return false
  */
  
  llvm::BasicBlock *ltBlock = builder.makeBlock();
  llvm::BasicBlock *geBlock = builder.makeBlock();
  llvm::BasicBlock *notEndBlock = builder.makeBlock();
  llvm::BasicBlock *notLtBlock = builder.makeBlock();
  llvm::Value *leftArr = builder.ir.CreateLoad(func->arg_begin());
  llvm::Value *rightArr = builder.ir.CreateLoad(func->arg_begin() + 1);
  ArrPairLoop comp = iterArrPair(builder, leftArr, rightArr, geBlock);
  
  llvm::Value *leftAtEnd = builder.ir.CreateICmpEQ(comp.left, comp.leftEnd);
  builder.ir.CreateCondBr(leftAtEnd, ltBlock, notEndBlock);
  
  builder.setCurr(notEndBlock);
  CompareExpr compare{data.inst, builder.ir};
  llvm::Value *lt = compare.less(arr->elem.get(), lvalue(comp.left), lvalue(comp.right));
  builder.ir.CreateCondBr(lt, ltBlock, notLtBlock);
  
  builder.setCurr(notLtBlock);
  llvm::Value *gt = compare.less(arr->elem.get(), lvalue(comp.right), lvalue(comp.left));
  builder.ir.CreateCondBr(gt, geBlock, comp.loop);
  
  builder.setCurr(ltBlock);
  returnBool(builder.ir, true);
  builder.setCurr(geBlock);
  returnBool(builder.ir, false);
  
  return func;
}
