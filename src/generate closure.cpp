//
//  generate closure.cpp
//  STELA
//
//  Created by Indi Kernick on 13/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "generate closure.hpp"

#include "gen types.hpp"
#include "gen helpers.hpp"
#include "generate type.hpp"
#include "generate stat.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

namespace {

constexpr unsigned clo_idx_fun = 0;
constexpr unsigned clo_idx_dat = 1;

// constexpr unsigned clodat_idx_ref = 0;
constexpr unsigned clodat_idx_dtor = 1;
constexpr unsigned clodat_idx_cap = 2;

void stubAttributes(llvm::Function *func) {
  const size_t args = func->arg_size();
  for (unsigned a = 0; a != args; ++a) {
    if (func->hasParamAttribute(a, llvm::Attribute::ReadOnly)) {
      func->removeParamAttr(a, llvm::Attribute::ReadOnly);
      func->addParamAttr(a, llvm::Attribute::ReadNone);
    }
    if (func->hasParamAttribute(a, llvm::Attribute::WriteOnly)) {
      func->removeParamAttr(a, llvm::Attribute::WriteOnly);
      func->addParamAttr(a, llvm::Attribute::ReadNone);
    }
  }
  func->addFnAttr(llvm::Attribute::NoReturn);
}

}

template <>
llvm::Function *stela::genFn<PFGI::clo_stub>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  const Signature sig = getSignature(*clo);
  llvm::FunctionType *fnType = generateSig(ctx, sig);
  llvm::Function *func = makeInternalFunc(data.mod, fnType, "clo_stub");
  assignAttrs(func, sig);
  stubAttributes(func);
  FuncBuilder builder{func};
  llvm::Function *panic = data.inst.get<FGI::panic>();
  callPanic(builder.ir, panic, "Calling null function pointer");
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_bool>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    llvm::Type::getInt1Ty(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_bool");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addAttribute(0, llvm::Attribute::ZExt);
  FuncBuilder builder{func};
  
  /*
  return obj.fun != stub
  */
  
  llvm::Value *objPtr = func->arg_begin();
  llvm::Value *objFun = loadStructElem(builder.ir, objPtr, clo_idx_fun);
  llvm::Value *stub = data.inst.get<PFGI::clo_stub>(clo);
  llvm::Value *notStub = builder.ir.CreateICmpNE(objFun, stub);
  builder.ir.CreateRet(notStub);
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_fun_ctor>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::Type *funTy = type->getStructElementType(clo_idx_fun);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo(), funTy}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_fun_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  obj.fun = fun
  obj.dat = null
  */
  
  llvm::Value *objPtr = func->arg_begin();
  llvm::Value *fun = func->arg_begin() + 1;
  llvm::Value *objFunPtr = builder.ir.CreateStructGEP(objPtr, clo_idx_fun);
  builder.ir.CreateStore(fun, objFunPtr);
  llvm::Value *objDatPtr = builder.ir.CreateStructGEP(objPtr, clo_idx_dat);
  setNull(builder.ir, objDatPtr);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_lam_ctor>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::Type *funTy = type->getStructElementType(clo_idx_fun);
  llvm::Type *datTy = type->getStructElementType(clo_idx_dat);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo(), funTy, voidPtrTy(ctx)}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_lam_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(2, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  obj.fun = fun
  obj.dat = dat
  */
  
  llvm::Value *objPtr = func->arg_begin();
  llvm::Value *fun = func->arg_begin() + 1;
  llvm::Value *dat = func->arg_begin() + 2;
  llvm::Value *objFunPtr = builder.ir.CreateStructGEP(objPtr, clo_idx_fun);
  builder.ir.CreateStore(fun, objFunPtr);
  llvm::Value *objDatPtr = builder.ir.CreateStructGEP(objPtr, clo_idx_dat);
  llvm::Value *opaqueDat = builder.ir.CreatePointerCast(dat, datTy);
  builder.ir.CreateStore(opaqueDat, objDatPtr);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_def_ctor>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_def_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  obj.fun = stub
  obj.dat = null
  */
  
  llvm::Value *objPtr = func->arg_begin();
  llvm::Value *objFunPtr = builder.ir.CreateStructGEP(objPtr, clo_idx_fun);
  llvm::Function *stub = data.inst.get<PFGI::clo_stub>(clo);
  builder.ir.CreateStore(stub, objFunPtr);
  llvm::Value *objDatPtr = builder.ir.CreateStructGEP(objPtr, clo_idx_dat);
  setNull(builder.ir, objDatPtr);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_dtor>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_dtor");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  if obj.dat != null
    ptr_dtor(obj.dat.dtor, bitcast obj.dat)
  */
  
  llvm::BasicBlock *dtorBlock = builder.makeBlock();
  llvm::BasicBlock *doneBlock = builder.makeBlock();
  llvm::Value *objPtr = func->arg_begin();
  llvm::Value *objDatPtr = builder.ir.CreateStructGEP(objPtr, clo_idx_dat);
  llvm::Value *objDat = builder.ir.CreateLoad(objDatPtr);
  llvm::Value *datNotNull = builder.ir.CreateIsNotNull(objDat);
  builder.ir.CreateCondBr(datNotNull, dtorBlock, doneBlock);
  
  builder.setCurr(dtorBlock);
  llvm::Value *objDatDtor = loadStructElem(builder.ir, objDat, clodat_idx_dtor);
  llvm::Value *objDatRefPtr = refPtrPtrCast(builder.ir, objDatPtr);
  llvm::Function *ptrDtor = data.inst.get<FGI::ptr_dtor>();
  builder.ir.CreateCall(ptrDtor, {objDatDtor, objDatRefPtr});
  builder.ir.CreateBr(doneBlock);
  
  builder.setCurr(doneBlock);
  builder.ir.CreateRetVoid();
  
  return func;
}

namespace {

void assignFun(llvm::IRBuilder<> &ir, llvm::Value *dstPtr, llvm::Value *srcPtr) {
  llvm::Value *dstFunPtr = ir.CreateStructGEP(dstPtr, clo_idx_fun);
  llvm::Value *srcFun = loadStructElem(ir, srcPtr, clo_idx_fun);
  ir.CreateStore(srcFun, dstFunPtr);
}

llvm::Value *getDatRefPtr(llvm::IRBuilder<> &ir, llvm::Value *cloPtr) {
  return refPtrPtrCast(ir, ir.CreateStructGEP(cloPtr, clo_idx_dat));
}

llvm::Function *cloCtor(
  InstData data,
  ast::FuncType *clo,
  llvm::Function *ptrCtor,
  const llvm::Twine &name
) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), name);
  assignBinaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  dst.fun = src.fun
  ptrCtor(bitcast dst.dat, bitcast src.dat)
  */
  
  llvm::Value *dstPtr = func->arg_begin();
  llvm::Value *srcPtr = func->arg_begin() + 1;
  assignFun(builder.ir, dstPtr, srcPtr);
  llvm::Value *dstDatRefPtr = getDatRefPtr(builder.ir, dstPtr);
  llvm::Value *srcDatRefPtr = getDatRefPtr(builder.ir, srcPtr);
  builder.ir.CreateCall(ptrCtor, {dstDatRefPtr, srcDatRefPtr});
  builder.ir.CreateRetVoid();
  
  return func;
}

llvm::Function *cloAsgn(
  InstData data,
  ast::FuncType *clo,
  llvm::Function *ptrAsgn,
  const llvm::Twine &name
) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), name);
  assignBinaryAliasCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  dst.fun = src.fun
  if dst.dat != null
    ptrAsgn(dst.dat.dtor, bitcast dst.dat, bitcast src.dat)
  */
  
  llvm::BasicBlock *asgnBlock = builder.makeBlock();
  llvm::BasicBlock *doneBlock = builder.makeBlock();
  llvm::Value *dstPtr = func->arg_begin();
  llvm::Value *srcPtr = func->arg_begin() + 1;
  assignFun(builder.ir, dstPtr, srcPtr);
  llvm::Value *dstDatPtr = builder.ir.CreateStructGEP(dstPtr, clo_idx_dat);
  llvm::Value *dstDat = builder.ir.CreateLoad(dstDatPtr);
  llvm::Value *datNotNull = builder.ir.CreateIsNotNull(dstDat);
  builder.ir.CreateCondBr(datNotNull, asgnBlock, doneBlock);
  
  builder.setCurr(asgnBlock);
  llvm::Value *dstDatDtor = loadStructElem(builder.ir, dstDat, clodat_idx_dtor);
  llvm::Value *dstDatRefPtr = refPtrPtrCast(builder.ir, dstDatPtr);
  llvm::Value *srcDatRefPtr = getDatRefPtr(builder.ir, srcPtr);
  builder.ir.CreateCall(ptrAsgn, {dstDatDtor, dstDatRefPtr, srcDatRefPtr});
  builder.ir.CreateBr(doneBlock);
  
  builder.setCurr(doneBlock);
  builder.ir.CreateRetVoid();
  
  return func;
}

}

template <>
llvm::Function *stela::genFn<PFGI::clo_cop_ctor>(InstData data, ast::FuncType *clo) {
  return cloCtor(data, clo, data.inst.get<FGI::ptr_cop_ctor>(), "clo_cop_ctor");
}

template <>
llvm::Function *stela::genFn<PFGI::clo_cop_asgn>(InstData data, ast::FuncType *clo) {
  return cloAsgn(data, clo, data.inst.get<FGI::ptr_cop_asgn>(), "clo_cop_asgn");
}

template <>
llvm::Function *stela::genFn<PFGI::clo_mov_ctor>(InstData data, ast::FuncType *clo) {
  return cloCtor(data, clo, data.inst.get<FGI::ptr_mov_ctor>(), "clo_mov_ctor");
}

template <>
llvm::Function *stela::genFn<PFGI::clo_mov_asgn>(InstData data, ast::FuncType *clo) {
  return cloAsgn(data, clo, data.inst.get<FGI::ptr_mov_asgn>(), "clo_mov_asgn");
}

template <>
llvm::Function *stela::genFn<PFGI::clo_eq>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = compareFor(type);
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_eq");
  assignCompareAttrs(func);
  FuncBuilder builder{func};
  
  /*
  return left.fun == right.fun
  */
  
  llvm::Value *leftPtr = func->arg_begin();
  llvm::Value *rightPtr = func->arg_begin() + 1;
  llvm::Value *leftFun = loadStructElem(builder.ir, leftPtr, clo_idx_fun);
  llvm::Value *rightFun = loadStructElem(builder.ir, rightPtr, clo_idx_fun);
  llvm::Value *equal = builder.ir.CreateICmpEQ(leftFun, rightFun);
  builder.ir.CreateRet(equal);
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_lt>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = compareFor(type);
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_lt");
  assignCompareAttrs(func);
  FuncBuilder builder{func};
  
  /*
  return left.fun < right.fun
  */
  
  llvm::Value *leftPtr = func->arg_begin();
  llvm::Value *rightPtr = func->arg_begin() + 1;
  llvm::Value *leftFun = loadStructElem(builder.ir, leftPtr, clo_idx_fun);
  llvm::Value *rightFun = loadStructElem(builder.ir, rightPtr, clo_idx_fun);
  llvm::Value *equal = builder.ir.CreateICmpULT(leftFun, rightFun);
  builder.ir.CreateRet(equal);
  
  return func;
}

llvm::Function *stela::genLambdaBody(gen::Ctx ctx, ast::Lambda &lam) {
  const Signature sig = getSignature(lam);
  llvm::FunctionType *fnType = generateSig(ctx.llvm, sig);
  llvm::Function *func = makeInternalFunc(ctx.mod, fnType, "lam_body");
  assignAttrs(func, sig);
  FuncBuilder builder{func};
  llvm::Type *capTy = generateLambdaCapture(ctx.llvm, lam)->getPointerTo();
  llvm::Value *funcCtx = builder.ir.CreatePointerCast(func->arg_begin(), capTy);
  ast::Receiver rec;
  generateStat(ctx, {builder, funcCtx}, rec, lam.params, lam.body);
  return func;
}

llvm::Value *stela::closureCapAddr(FuncBuilder &builder, llvm::Value *captures, const uint32_t index) {
  return builder.ir.CreateStructGEP(captures, clodat_idx_cap + index);
}

llvm::Value *stela::closureFun(FuncBuilder &builder, llvm::Value *closure) {
  return loadStructElem(builder.ir, closure, clo_idx_fun);
}

llvm::Value *stela::closureDat(FuncBuilder &builder, llvm::Value *closure) {
  llvm::Value *dat = loadStructElem(builder.ir, closure, clo_idx_dat);
  return builder.ir.CreatePointerCast(dat, voidPtrTy(closure->getContext()));
}

llvm::Function *stela::getVirtualDtor(gen::Ctx ctx, const ast::Lambda &lambda) {
  llvm::Function *func = makeInternalFunc(ctx.mod, dtorTy(ctx.llvm), "clo_cap_dtor");
  FuncBuilder builder{func};
  llvm::Type *clodatTy = generateLambdaCapture(ctx.llvm, lambda);
  llvm::Type *clodatPtrTy = clodatTy->getPointerTo();
  llvm::Value *clodat = builder.ir.CreatePointerCast(func->arg_begin(), clodatPtrTy);
  LifetimeExpr lifetime{ctx.inst, builder.ir};
  const std::vector<sym::ClosureCap> &caps = lambda.symbol->captures;
  for (uint32_t c = 0; c != caps.size(); ++c) {
    lifetime.destroy(caps[c].type.get(), closureCapAddr(builder, clodat, c));
  }
  builder.ir.CreateRetVoid();
  return func;
}

void stela::setDtor(FuncBuilder &builder, llvm::Value *captures, llvm::Function *dtor) {
  llvm::Value *dtorPtr = builder.ir.CreateStructGEP(captures, clodat_idx_dtor);
  builder.ir.CreateStore(dtor, dtorPtr);
}
