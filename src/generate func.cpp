//
//  generate func.cpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate func.hpp"

#include "unreachable.hpp"
#include "type builder.hpp"
#include "generate type.hpp"
#include "generate decl.hpp"
#include <llvm/IR/Function.h>
#include "assert down cast.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

std::string stela::generateNullFunc(gen::Ctx ctx, const ast::FuncType &type) {
  /*gen::String name;
  name += "f_null_";
  // @TODO maybe consider caching type names
  // generateFuncName does that same work that generateNullFunc does
  name += generateFuncName(ctx, type);
  if (ctx.inst.funcNotInst(name)) {
    ctx.func += "[[noreturn]] static ";
    ctx.func += generateType(ctx, type.ret.get());
    ctx.func += ' ';
    ctx.func += name;
    ctx.func += "(void *";
    for (const ast::ParamType &param : type.params) {
      ctx.func += ", ";
      ctx.func += generateType(ctx, param.type.get());
      if (param.ref == ast::ParamRef::ref) {
        ctx.func += " &";
      }
    }
    ctx.func += ") noexcept {\n";
    ctx.func += "panic(\"Calling null function pointer\");\n";
    ctx.func += "}\n";
  }
  return name;*/
  return {};
}

std::string stela::generateMakeFunc(gen::Ctx ctx, ast::FuncType &type) {
  /*gen::String name;
  name += "f_makefunc_";
  name += generateFuncName(ctx, type);
  if (ctx.inst.funcNotInst(name)) {
    ctx.func += "static ";
    ctx.func += generateType(ctx, &type);
    ctx.func += " ";
    ctx.func += name;
    ctx.func += "(const ";
    ctx.func += generateFuncSig(ctx, type);
    ctx.func += R"( func) noexcept {
auto *ptr = static_cast<FuncClosureData *>(allocate(sizeof(FuncClosureData)));
new (ptr) FuncClosureData(); // setup virtual destructor
ptr->count = 1;
return {func, ClosureDataPtr{static_cast<ClosureData *>(ptr)}};
}
)";
  }
  return name;*/
  return {};
}

std::string stela::generateLambda(gen::Ctx ctx, const ast::Lambda &lambda) {
  /*gen::String func;
  func += "static ";
  func += generateType(ctx, lambda.ret.get());
  func += " ";
  gen::String name;
  name += "f_lam_";
  name += lambda.id;
  func += name;
  func += "(";
  func += generateLambdaCapture(ctx, lambda);
  func += " &capture";
  sym::Lambda *symbol = lambda.symbol;
  for (size_t p = 0; p != symbol->params.size(); ++p) {
    func += ", ";
    func += generateType(ctx, symbol->params[p].type.get());
    func += ' ';
    if (symbol->params[p].ref == sym::ValueRef::ref) {
      func += '&';
    }
    func += "p_";
    func += (p + 1);
  }
  func += ") noexcept {\n";
  func += generateDecl(ctx, lambda.body);
  func += "}\n";
  ctx.func += func;
  return name;*/
  return {};
}

std::string stela::generateMakeLam(gen::Ctx ctx, const ast::Lambda &lambda) {
  /*gen::String func;
  func += "static inline ";
  func += generateType(ctx, lambda.exprType.get());
  func += " ";
  gen::String name;
  name += "f_makelam_";
  name += lambda.id;
  func += name;
  func += "(";
  const gen::String capture = generateLambdaCapture(ctx, lambda);
  func += capture;
  func += " capture) noexcept {\n";
  
  func += "auto *ptr = static_cast<";
  func += capture;
  func += " *>(allocate(sizeof(";
  func += capture;
  func += ")));\n";
  
  func += "new (ptr) ";
  func += capture;
  func += "(capture);\n";
  
  func += "ptr->count = 1;\n";
  
  func += "return {reinterpret_cast<";
  auto *funcType = assertDownCast<ast::FuncType>(lambda.exprType.get());
  func += generateFuncSig(ctx, *funcType);
  func += ">(";
  func += generateLambda(ctx, lambda);
  func += "), ClosureDataPtr{static_cast<ClosureData *>(ptr)}};\n";
  
  func += "}\n";
  
  ctx.func += func;
  return name;*/
  return {};
}

namespace {

constexpr size_t array_idx_ref = 0;
constexpr size_t array_idx_cap = 1;
constexpr size_t array_idx_len = 2;
constexpr size_t array_idx_dat = 3;

llvm::Function *makeInternalFunc(
  llvm::Module *module,
  llvm::FunctionType *type,
  const llvm::Twine &name
) {
  llvm::Function *func = llvm::Function::Create(
    type,
    llvm::Function::ExternalLinkage, // @TODO temporary. So functions aren't removed
    name,
    module
  );
  func->addFnAttr(llvm::Attribute::NoUnwind);
  func->addFnAttr(llvm::Attribute::AlwaysInline);
  return func;
}

llvm::Function *declareCFunc(
  llvm::Module *module,
  llvm::FunctionType *type,
  const llvm::Twine &name
) {
  llvm::Function *func = llvm::Function::Create(
    type,
    llvm::Function::ExternalLinkage,
    name,
    module
  );
  func->addFnAttr(llvm::Attribute::NoUnwind);
  return func;
}

llvm::FunctionType *dtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getVoidTy(type->getContext()),
    {type},
    false
  );
}

llvm::FunctionType *defCtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getVoidTy(type->getContext()),
    {type->getPointerTo()},
    false
  );
}

llvm::FunctionType *copCtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(type, {type}, false);
}

llvm::FunctionType *copAsgnFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getVoidTy(type->getContext()),
    {type->getPointerTo(), type},
    false
  );
}

llvm::FunctionType *movCtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getVoidTy(type->getContext()),
    {type->getPointerTo(), type->getPointerTo()},
    false
  );
}

llvm::FunctionType *movAsgnFor(llvm::Type *type) {
  return movCtorFor(type);
}

llvm::Value *wrapPtr(llvm::IRBuilder<> &ir, llvm::Value *ptr) {
  TypeBuilder typeBdr{ir.getContext()};
  llvm::Value *undef = llvm::UndefValue::get(typeBdr.wrap(ptr->getType()));
  return ir.CreateInsertValue(undef, ptr, {0u});
}

llvm::Constant *constantFor(llvm::Value *val, const uint64_t value) {
  return llvm::ConstantInt::get(val->getType(), value);
}

void setNull(llvm::IRBuilder<> &ir, llvm::Value *ptrToPtr) {
  llvm::Type *dstType = ptrToPtr->getType()->getPointerElementType();
  llvm::PointerType *dstPtrType = llvm::dyn_cast<llvm::PointerType>(dstType);
  llvm::Value *null = llvm::ConstantPointerNull::get(dstPtrType);
  ir.CreateStore(null, ptrToPtr);
}

void refIncr(llvm::IRBuilder<> &ir, llvm::Value *objPtr) {
  llvm::Value *refPtr = ir.CreateStructGEP(objPtr, array_idx_ref);
  llvm::Value *value = ir.CreateLoad(refPtr);
  llvm::Constant *one = constantFor(value, 1);
  llvm::Value *added = ir.CreateNSWAdd(value, one);
  ir.CreateStore(added, refPtr);
}

llvm::Value *refDecr(llvm::IRBuilder<> &ir, llvm::Value *objPtr) {
  llvm::Value *refPtr = ir.CreateStructGEP(objPtr, array_idx_ref);
  llvm::Value *value = ir.CreateLoad(refPtr);
  llvm::Constant *one = constantFor(value, 1);
  llvm::Value *subed = ir.CreateNSWSub(value, one);
  ir.CreateStore(subed, refPtr);
  return ir.CreateICmpEQ(subed, constantFor(value, 0));
}

void resetArray(gen::FuncInst &inst, FuncBuilder &funcBdr, llvm::Value *array, llvm::BasicBlock *doneBlock) {
  llvm::BasicBlock *deleteBlock = funcBdr.makeBlock();
  // Decrement reference count
  llvm::Value *deleteArray = refDecr(funcBdr.ir, array);
  // If reference count reached zero...
  funcBdr.ir.CreateCondBr(deleteArray, deleteBlock, doneBlock);
  funcBdr.setCurr(deleteBlock);
  // Deallocate
  funcBdr.callFree(inst.free(), array);
  funcBdr.ir.CreateBr(doneBlock);
}

void likely(llvm::BranchInst *branch) {
  llvm::LLVMContext &ctx = branch->getContext();
  llvm::IntegerType *i32 = llvm::IntegerType::getInt32Ty(ctx);
  // likely to be not null
  llvm::ConstantInt *trueWeight = llvm::ConstantInt::get(i32, 2000);
  llvm::Metadata *trueMeta = llvm::ConstantAsMetadata::get(trueWeight);
  // unlikely to be null
  llvm::ConstantInt *falseWeight = llvm::ConstantInt::get(i32, 1);
  llvm::Metadata *falseMeta = llvm::ConstantAsMetadata::get(falseWeight);
  llvm::MDTuple *tuple = llvm::MDNode::get(ctx, {
    llvm::MDString::get(ctx, "branch_weights"), trueMeta, falseMeta
  });
  branch->setMetadata("prof", tuple);
}

void callPanic(llvm::IRBuilder<> &ir, llvm::Function *panic, std::string_view message) {
  ir.CreateCall(panic, ir.CreateGlobalStringPtr({message.data(), message.size()}));
  ir.CreateUnreachable();
}

}

template <>
llvm::IntegerType *stela::getSizedType<1>(llvm::LLVMContext &ctx) {
  return llvm::IntegerType::getInt8Ty(ctx);
}

template <>
llvm::IntegerType *stela::getSizedType<2>(llvm::LLVMContext &ctx) {
  return llvm::IntegerType::getInt16Ty(ctx);
}

template <>
llvm::IntegerType *stela::getSizedType<4>(llvm::LLVMContext &ctx) {
  return llvm::IntegerType::getInt32Ty(ctx);
}

template <>
llvm::IntegerType *stela::getSizedType<8>(llvm::LLVMContext &ctx) {
  return llvm::IntegerType::getInt64Ty(ctx);
}

llvm::Function *stela::generatePanic(llvm::Module *module) {
  llvm::LLVMContext &ctx = module->getContext();
  
  llvm::Type *voidTy = llvm::Type::getVoidTy(ctx);
  llvm::Type *strTy = getTypePtr<char>(ctx);
  llvm::Type *intTy = getType<int>(ctx);
  llvm::FunctionType *putsType = llvm::FunctionType::get(intTy, {strTy}, false);
  llvm::FunctionType *exitType = llvm::FunctionType::get(voidTy, {intTy}, false);
  llvm::FunctionType *panicType = llvm::FunctionType::get(voidTy, {strTy}, false);
  
  llvm::Function *puts = declareCFunc(module, putsType, "puts");
  llvm::Function *exit = declareCFunc(module, exitType, "exit");
  llvm::Function *panic = makeInternalFunc(module, panicType, "panic");
  puts->addParamAttr(0, llvm::Attribute::ReadOnly);
  puts->addParamAttr(0, llvm::Attribute::NoCapture);
  exit->addFnAttr(llvm::Attribute::NoReturn);
  panic->addFnAttr(llvm::Attribute::NoReturn);
  
  FuncBuilder funcBdr{panic};
  funcBdr.ir.CreateCall(puts, panic->arg_begin());
  funcBdr.ir.CreateCall(exit, llvm::ConstantInt::get(intTy, EXIT_FAILURE));
  funcBdr.ir.CreateUnreachable();
  
  return panic;
}

llvm::Function *stela::generateAlloc(gen::FuncInst &inst, llvm::Module *module) {
  llvm::LLVMContext &ctx = module->getContext();
  
  llvm::Type *memTy = llvm::Type::getInt8PtrTy(ctx);
  llvm::Type *sizeTy = getType<size_t>(ctx);
  llvm::FunctionType *mallocType = llvm::FunctionType::get(memTy, {sizeTy}, false);
  llvm::FunctionType *allocType = mallocType;
  
  llvm::Function *malloc = declareCFunc(module, mallocType, "malloc");
  llvm::Function *panic = inst.panic();
  llvm::Function *alloc = makeInternalFunc(module, allocType, "alloc");
  malloc->addAttribute(0, llvm::Attribute::NoAlias);
  malloc->addFnAttr(llvm::Attribute::NoUnwind);
  alloc->addAttribute(0, llvm::Attribute::NoAlias);
  
  FuncBuilder funcBdr{alloc};
  llvm::BasicBlock *okBlock = funcBdr.makeBlock();
  llvm::BasicBlock *errorBlock = funcBdr.makeBlock();
  llvm::Value *ptr = funcBdr.ir.CreateCall(malloc, alloc->arg_begin());
  llvm::Value *isNotNull = funcBdr.ir.CreateIsNotNull(ptr);
  likely(funcBdr.ir.CreateCondBr(isNotNull, okBlock, errorBlock));
  
  funcBdr.setCurr(okBlock);
  funcBdr.ir.CreateRet(ptr);
  funcBdr.setCurr(errorBlock);
  callPanic(funcBdr.ir, panic, "Out of memory");
  
  return alloc;
}

llvm::Function *stela::generateFree(llvm::Module *module) {
  llvm::LLVMContext &ctx = module->getContext();
  llvm::Type *voidTy = llvm::Type::getVoidTy(ctx);
  llvm::Type *memTy = llvm::Type::getInt8PtrTy(ctx);
  llvm::FunctionType *freeType = llvm::FunctionType::get(voidTy, {memTy}, false);
  llvm::Function *free = declareCFunc(module, freeType, "free");
  free->addParamAttr(0, llvm::Attribute::NoCapture);
  return free;
}

// @FIXME pointer parameter
llvm::Function *stela::generateArrayDtor(
  gen::FuncInst &inst, llvm::Module *module, llvm::Type *type
) {
  llvm::Function *func = makeInternalFunc(module, dtorFor(type), "array_dtor");
  FuncBuilder funcBdr{func};
  llvm::BasicBlock *decrBlock = funcBdr.makeBlock();
  llvm::BasicBlock *doneBlock = funcBdr.makeBlock();
  llvm::Value *array = funcBdr.ir.CreateExtractValue(func->arg_begin(), {0});
  funcBdr.ir.CreateCondBr(funcBdr.ir.CreateIsNull(array), doneBlock, decrBlock);
  funcBdr.setCurr(decrBlock);
  resetArray(inst, funcBdr, array, doneBlock);
  funcBdr.setCurr(doneBlock);
  funcBdr.ir.CreateRetVoid();
  return func;
}





// this wrapping and unwrapping is kind of confusing. Sort it out





llvm::Function *stela::generateArrayDefCtor(
  gen::FuncInst &inst, llvm::Module *module, llvm::Type *type
) {
  llvm::Function *func = makeInternalFunc(module, defCtorFor(type), "array_def_ctor");
  FuncBuilder funcBdr{func};
  TypeBuilder typeBdr{type->getContext()};
  llvm::Type *arrayPtrType = type->getArrayElementType();
  llvm::Type *arrayStructType = arrayPtrType->getPointerElementType();
  llvm::Value *array = funcBdr.callAlloc(inst.alloc(), arrayStructType);

  /*
  ref = 1
  cap = 0
  len = 0
  dat = null
  */

  llvm::Value *ref = funcBdr.ir.CreateStructGEP(array, array_idx_ref);
  funcBdr.ir.CreateStore(llvm::ConstantInt::get(typeBdr.ref(), 1), ref);
  llvm::Value *cap = funcBdr.ir.CreateStructGEP(array, array_idx_cap);
  funcBdr.ir.CreateStore(llvm::ConstantInt::get(typeBdr.len(), 0), cap);
  llvm::Value *len = funcBdr.ir.CreateStructGEP(array, array_idx_len);
  funcBdr.ir.CreateStore(llvm::ConstantInt::get(typeBdr.len(), 0), len);
  llvm::Value *dat = funcBdr.ir.CreateStructGEP(array, array_idx_dat);
  llvm::Type *elem = arrayStructType->getStructElementType(array_idx_dat);
  funcBdr.ir.CreateStore(typeBdr.nullPtrTo(elem->getPointerElementType()), dat);
  funcBdr.ir.CreateStore(wrapPtr(funcBdr.ir, array), func->arg_begin());
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

// @FIXME two pointer parameters
llvm::Function *stela::generateArrayCopCtor(
  gen::FuncInst &, llvm::Module *module, llvm::Type *type
) {
  llvm::Function *func = makeInternalFunc(module, copCtorFor(type), "array_cop_ctor");
  FuncBuilder funcBdr{func};
  
  /*
  increment other
  return other
  */
  
  llvm::Value *array = funcBdr.ir.CreateExtractValue(func->arg_begin(), {0});
  refIncr(funcBdr.ir, array);
  funcBdr.ir.CreateRet(wrapPtr(funcBdr.ir, array));
  return func;
}

llvm::Function *stela::generateArrayCopAsgn(
  gen::FuncInst &inst, llvm::Module *module, llvm::Type *type
) {
  llvm::Function *func = makeInternalFunc(module, copAsgnFor(type), "array_cop_asgn");
  FuncBuilder funcBdr{func};
  
  /*
  increment right
  decrement left
  copy right to left
  */
  
  llvm::BasicBlock *block = funcBdr.makeBlock();
  llvm::Value *right = funcBdr.ir.CreateExtractValue(func->arg_begin() + 1, {0});
  refIncr(funcBdr.ir, right);
  llvm::Value *leftPtr = func->arg_begin();
  llvm::Value *leftWrapper = funcBdr.ir.CreateLoad(leftPtr);
  resetArray(inst, funcBdr, funcBdr.ir.CreateExtractValue(leftWrapper, {0}), block);
  funcBdr.setCurr(block);
  funcBdr.ir.CreateStore(func->arg_begin() + 1, leftPtr);
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::generateArrayMovCtor(
  gen::FuncInst &, llvm::Module *module, llvm::Type *type
) {
  llvm::Function *func = makeInternalFunc(module, movCtorFor(type), "array_mov_ctor");
  FuncBuilder funcBdr{func};

  llvm::Value *leftPtr = funcBdr.ir.CreateStructGEP(func->arg_begin(), 0);
  llvm::Value *rightPtr = funcBdr.ir.CreateStructGEP(func->arg_begin() + 1, 0);
  funcBdr.ir.CreateStore(funcBdr.ir.CreateLoad(rightPtr), leftPtr);
  setNull(funcBdr.ir, rightPtr);
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::generateArrayMovAsgn(
  gen::FuncInst &inst, llvm::Module *module, llvm::Type *type
) {
  llvm::Function *func = makeInternalFunc(module, movAsgnFor(type), "array_mov_asgn");
  FuncBuilder funcBdr{func};
  
  llvm::BasicBlock *block = funcBdr.makeBlock();
  llvm::Value *leftPtr = funcBdr.ir.CreateStructGEP(func->arg_begin(), 0);
  resetArray(inst, funcBdr, funcBdr.ir.CreateLoad(leftPtr), block);
  funcBdr.setCurr(block);
  llvm::Value *rightPtr = funcBdr.ir.CreateStructGEP(func->arg_begin() + 1, 0);
  funcBdr.ir.CreateStore(funcBdr.ir.CreateLoad(rightPtr), leftPtr);
  setNull(funcBdr.ir, rightPtr);
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

namespace {

using BoundsChecker = llvm::Value *(llvm::IRBuilder<> &, llvm::Value *, llvm::Value *);

llvm::Function *generateArrayIdx(
  gen::FuncInst &inst,
  llvm::Module *module,
  llvm::Type *type,
  BoundsChecker *checkBounds,
  const llvm::Twine &name
) {
  llvm::Type *arrayStructPtr = type->getArrayElementType();
  llvm::Type *arrayStruct = arrayStructPtr->getPointerElementType();
  llvm::Type *elemPtr = arrayStruct->getStructElementType(array_idx_dat);
  llvm::Type *i32 = llvm::Type::getInt32Ty(type->getContext());
  llvm::Type *sizeTy = getType<size_t>(type->getContext());
  llvm::FunctionType *fnType = llvm::FunctionType::get(elemPtr, {type, i32}, false);
  llvm::Function *func = makeInternalFunc(module, fnType, name);
  FuncBuilder funcBdr{func};
  
  llvm::BasicBlock *okBlock = funcBdr.makeBlock();
  llvm::BasicBlock *errorBlock = funcBdr.makeBlock();
  llvm::Value *arrayPtr = funcBdr.ir.CreateExtractValue(func->arg_begin(), {0});
  llvm::Value *size = funcBdr.ir.CreateLoad(
    funcBdr.ir.CreateStructGEP(arrayPtr, array_idx_len)
  );
  llvm::Value *inBounds = checkBounds(funcBdr.ir, func->arg_begin() + 1, size);
  likely(funcBdr.ir.CreateCondBr(inBounds, okBlock, errorBlock));
  
  funcBdr.setCurr(okBlock);
  llvm::Value *idx = funcBdr.ir.CreateIntCast(func->arg_begin() + 1, sizeTy, false);
  llvm::Value *data = funcBdr.ir.CreateLoad(
    funcBdr.ir.CreateStructGEP(arrayPtr, array_idx_dat)
  );
  llvm::Value *elem = funcBdr.ir.CreateInBoundsGEP(elemPtr->getPointerElementType(), data, idx);
  funcBdr.ir.CreateRet(elem);
  
  funcBdr.setCurr(errorBlock);
  callPanic(funcBdr.ir, inst.panic(), "Index out of bounds");
  
  return func;
}

llvm::Value *checkSignedBounds(llvm::IRBuilder<> &ir, llvm::Value *index, llvm::Value *size) {
  llvm::Type *i32 = llvm::Type::getInt32Ty(ir.getContext());
  llvm::Value *zero = llvm::ConstantInt::get(i32, 0, true);
  llvm::Value *aboveZero = ir.CreateICmpSGE(index, zero);
  llvm::Value *belowSize = ir.CreateICmpSLT(index, size);
  return ir.CreateAnd(aboveZero, belowSize);
}

llvm::Value *checkUnsignedBounds(llvm::IRBuilder<> &ir, llvm::Value *index, llvm::Value *size) {
  return ir.CreateICmpULT(index, size);
}

}

llvm::Function *stela::generateArrayIdxS(
  gen::FuncInst &inst, llvm::Module *module, llvm::Type *type
) {
  return generateArrayIdx(inst, module, type, checkSignedBounds, "array_idx_s");
}

llvm::Function *stela::generateArrayIdxU(
  gen::FuncInst &inst, llvm::Module *module, llvm::Type *type
) {
  return generateArrayIdx(inst, module, type, checkUnsignedBounds, "array_idx_u");
}
