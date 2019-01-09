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
#include "compare exprs.hpp"
#include "lifetime exprs.hpp"
#include <llvm/IR/Function.h>
#include "assert down cast.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

// stub
std::string stela::generateNullFunc(gen::Ctx ctx, const ast::FuncType &type) {
  /*gen::String name;
  name += "f_null_";
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

llvm::FunctionType *unaryCtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getVoidTy(type->getContext()),
    {type->getPointerTo()},
    false
  );
}

llvm::FunctionType *binaryCtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getVoidTy(type->getContext()),
    {type->getPointerTo(), type->getPointerTo()},
    false
  );
}

llvm::FunctionType *compareFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getInt1Ty(type->getContext()),
    {type->getPointerTo(), type->getPointerTo()},
    false
  );
}

llvm::Constant *constantFor(llvm::Type *type, const uint64_t value) {
  return llvm::ConstantInt::get(type, value);
}

llvm::Constant *constantFor(llvm::Value *val, const uint64_t value) {
  return constantFor(val->getType(), value);
}

llvm::Constant *constantForPtr(llvm::Value *ptrToVal, const uint64_t value) {
  return constantFor(ptrToVal->getType()->getPointerElementType(), value);
}

void callPanic(llvm::IRBuilder<> &ir, llvm::Function *panic, std::string_view message) {
  ir.CreateCall(panic, ir.CreateGlobalStringPtr({message.data(), message.size()}));
  ir.CreateUnreachable();
}

llvm::Value *callAlloc(llvm::IRBuilder<> &ir, llvm::Function *alloc, llvm::Type *type, llvm::Value *count) {
  llvm::LLVMContext &ctx = alloc->getContext();
  llvm::Type *sizeTy = getType<size_t>(ctx);
  llvm::Constant *size64 = llvm::ConstantExpr::getSizeOf(type);
  llvm::Constant *size = llvm::ConstantExpr::getIntegerCast(size64, sizeTy, false);
  llvm::Value *numElems = ir.CreateIntCast(count, sizeTy, false);
  llvm::Value *bytes = ir.CreateMul(size, numElems);
  llvm::Value *memPtr = ir.CreateCall(alloc, {bytes});
  return ir.CreatePointerCast(memPtr, type->getPointerTo());
}

llvm::Value *callAlloc(llvm::IRBuilder<> &ir, llvm::Function *alloc, llvm::Type *type) {
  llvm::Type *sizeTy = getType<size_t>(alloc->getContext());
  return callAlloc(ir, alloc, type, llvm::ConstantInt::get(sizeTy, 1));
}

void callFree(llvm::IRBuilder<> &ir, llvm::Function *free, llvm::Value *ptr) {
  llvm::Type *i8ptr = llvm::Type::getInt8PtrTy(ptr->getContext());
  ir.CreateCall(free, ir.CreatePointerCast(ptr, i8ptr));
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
  callFree(funcBdr.ir, inst.free(), array);
  funcBdr.ir.CreateBr(doneBlock);
}

void resetPtr(
  gen::FuncInst &inst,
  FuncBuilder &funcBdr,
  llvm::Value *ptr,
  llvm::Function *dtor,
  llvm::BasicBlock *done
) {
  /*
  ptr.ref--
  if ptr.ref == 0
    dtor
    free ptr
    done
  else
    done
  */
  llvm::BasicBlock *del = funcBdr.makeBlock();
  funcBdr.ir.CreateCondBr(refDecr(funcBdr.ir, ptr), del, done);
  funcBdr.setCurr(del);
  funcBdr.ir.CreateCall(dtor, {ptr});
  callFree(funcBdr.ir, inst.free(), ptr);
  funcBdr.ir.CreateBr(done);
  funcBdr.setCurr(done);
}

llvm::Value *ptrDefCtor(
  gen::FuncInst &inst, FuncBuilder &funcBdr, llvm::Type *elemType
) {
  /*
  ptr = malloc elemType
  ptr.ref = 1
  */
  llvm::Value *ptr = callAlloc(funcBdr.ir, inst.alloc(), elemType);
  llvm::Value *ref = funcBdr.ir.CreateStructGEP(ptr, array_idx_ref);
  TypeBuilder typeBdr{ptr->getContext()};
  funcBdr.ir.CreateStore(llvm::ConstantInt::get(typeBdr.ref(), 1), ref);
  return ptr;
}

void ptrDtor(
  gen::FuncInst &inst,
  FuncBuilder &funcBdr,
  llvm::Value *objPtr,
  llvm::Function *dtor,
  llvm::BasicBlock *done
) {
  /*
  if ptr == null
    done
  else
    reset ptr
    done
  */
  llvm::BasicBlock *decr = funcBdr.makeBlock();
  llvm::Value *ptr = funcBdr.ir.CreateLoad(objPtr);
  funcBdr.ir.CreateCondBr(funcBdr.ir.CreateIsNull(ptr), done, decr);
  funcBdr.setCurr(decr);
  resetPtr(inst, funcBdr, ptr, dtor, done);
}

void ptrCopCtor(
  FuncBuilder &funcBdr, llvm::Value *objPtr, llvm::Value *otherPtr
) {
  /*
  other.ref++
  obj = other
  */
  llvm::Value *other = funcBdr.ir.CreateLoad(otherPtr);
  refIncr(funcBdr.ir, other);
  funcBdr.ir.CreateStore(other, objPtr);
}

void ptrMovCtor(
  FuncBuilder &funcBdr, llvm::Value *objPtr, llvm::Value *otherPtr
) {
  /*
  obj = other
  other = null
  */
  funcBdr.ir.CreateStore(funcBdr.ir.CreateLoad(otherPtr), objPtr);
  setNull(funcBdr.ir, otherPtr);
}

void ptrCopAsgn(
  gen::FuncInst &inst,
  FuncBuilder &funcBdr,
  llvm::Value *leftPtr,
  llvm::Value *rightPtr,
  llvm::Function *dtor,
  llvm::BasicBlock *done
) {
  /*
  right.ref++
  reset left
  left = right
  */
  llvm::BasicBlock *innerDone = funcBdr.makeBlock();
  llvm::Value *right = funcBdr.ir.CreateLoad(rightPtr);
  refIncr(funcBdr.ir, right);
  llvm::Value *left = funcBdr.ir.CreateLoad(leftPtr);
  resetPtr(inst, funcBdr, left, dtor, innerDone);
  funcBdr.ir.CreateStore(right, leftPtr);
  funcBdr.ir.CreateBr(done);
  funcBdr.setCurr(done);
}

void ptrMovAsgn(
  gen::FuncInst &inst,
  FuncBuilder &funcBdr,
  llvm::Value *leftPtr,
  llvm::Value *rightPtr,
  llvm::Function *dtor,
  llvm::BasicBlock *done
) {
  /*
  reset left
  left = right
  right = null
  */
  llvm::BasicBlock *innerDone = funcBdr.makeBlock();
  llvm::Value *left = funcBdr.ir.CreateLoad(leftPtr);
  resetPtr(inst, funcBdr, left, dtor, innerDone);
  funcBdr.ir.CreateStore(funcBdr.ir.CreateLoad(rightPtr), leftPtr);
  setNull(funcBdr.ir, rightPtr);
  funcBdr.ir.CreateBr(done);
  funcBdr.setCurr(done);
}

void likely(llvm::BranchInst *branch) {
  llvm::LLVMContext &ctx = branch->getContext();
  llvm::IntegerType *i32 = llvm::IntegerType::getInt32Ty(ctx);
  llvm::ConstantInt *trooWeight = llvm::ConstantInt::get(i32, 2048);
  llvm::Metadata *trooMeta = llvm::ConstantAsMetadata::get(trooWeight);
  llvm::ConstantInt *folsWeight = llvm::ConstantInt::get(i32, 1);
  llvm::Metadata *folsMeta = llvm::ConstantAsMetadata::get(folsWeight);
  llvm::MDTuple *tuple = llvm::MDNode::get(ctx, {
    llvm::MDString::get(ctx, "branch_weights"), trooMeta, folsMeta
  });
  branch->setMetadata("prof", tuple);
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
  exit->addFnAttr(llvm::Attribute::Cold);
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

llvm::Function *stela::genArrDtor(
  gen::FuncInst &inst, llvm::Module *module, ast::ArrayType *arr
) {
  llvm::Type *type = generateType(module->getContext(), arr);
  llvm::Function *func = makeInternalFunc(module, unaryCtorFor(type), "arr_dtor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  llvm::BasicBlock *done = funcBdr.makeBlock();
  ptrDtor(inst, funcBdr, func->arg_begin(), inst.arrayStrgDtor(arr), done);
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrDefCtor(
  gen::FuncInst &inst, llvm::Module *module, ast::ArrayType *arr
) {
  llvm::Type *type = generateType(module->getContext(), arr);
  llvm::Function *func = makeInternalFunc(module, unaryCtorFor(type), "arr_def_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  llvm::Type *arrayStructType = type->getPointerElementType();
  llvm::Value *array = ptrDefCtor(inst, funcBdr, arrayStructType);

  /*
  ref = 1
  cap = 0
  len = 0
  dat = null
  */

  llvm::Value *cap = funcBdr.ir.CreateStructGEP(array, array_idx_cap);
  funcBdr.ir.CreateStore(constantForPtr(cap, 0), cap);
  llvm::Value *len = funcBdr.ir.CreateStructGEP(array, array_idx_len);
  funcBdr.ir.CreateStore(constantForPtr(len, 0), len);
  llvm::Value *dat = funcBdr.ir.CreateStructGEP(array, array_idx_dat);
  setNull(funcBdr.ir, dat);
  funcBdr.ir.CreateStore(array, func->arg_begin());
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrCopCtor(
  gen::FuncInst &, llvm::Module *module, ast::ArrayType *arr
) {
  llvm::Type *type = generateType(module->getContext(), arr);
  llvm::Function *func = makeInternalFunc(module, binaryCtorFor(type), "arr_cop_ctor");
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NoAlias);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  ptrCopCtor(funcBdr, func->arg_begin(), func->arg_begin() + 1);
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrCopAsgn(
  gen::FuncInst &inst, llvm::Module *module, ast::ArrayType *arr
) {
  llvm::Type *type = generateType(module->getContext(), arr);
  llvm::Function *func = makeInternalFunc(module, binaryCtorFor(type), "arr_cop_asgn");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  llvm::BasicBlock *done = funcBdr.makeBlock();
  ptrCopAsgn(inst, funcBdr, func->arg_begin(), func->arg_begin() + 1, inst.arrayStrgDtor(arr), done);
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrMovCtor(
  gen::FuncInst &, llvm::Module *module, ast::ArrayType *arr
) {
  llvm::Type *type = generateType(module->getContext(), arr);
  llvm::Function *func = makeInternalFunc(module, binaryCtorFor(type), "arr_mov_ctor");
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NoAlias);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  ptrMovCtor(funcBdr, func->arg_begin(), func->arg_begin() + 1);
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrMovAsgn(
  gen::FuncInst &inst, llvm::Module *module, ast::ArrayType *arr
) {
  llvm::Type *type = generateType(module->getContext(), arr);
  llvm::Function *func = makeInternalFunc(module, binaryCtorFor(type), "arr_mov_asgn");
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NoAlias);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  llvm::BasicBlock *done = funcBdr.makeBlock();
  ptrMovAsgn(inst, funcBdr, func->arg_begin(), func->arg_begin() + 1, inst.arrayStrgDtor(arr), done);
  funcBdr.ir.CreateRetVoid();
  return func;
}

namespace {

using BoundsChecker = llvm::Value *(llvm::IRBuilder<> &, llvm::Value *, llvm::Value *);

llvm::Function *generateArrayIdx(
  gen::FuncInst &inst,
  llvm::Module *module,
  ast::ArrayType *arr,
  BoundsChecker *checkBounds,
  const llvm::Twine &name
) {
  llvm::Type *type = generateType(module->getContext(), arr);
  llvm::Type *arrayStruct = type->getPointerElementType();
  llvm::Type *elemPtr = arrayStruct->getStructElementType(array_idx_dat);
  llvm::Type *i32 = llvm::Type::getInt32Ty(type->getContext());
  llvm::Type *sizeTy = getType<size_t>(type->getContext());
  llvm::FunctionType *fnType = llvm::FunctionType::get(
    elemPtr,
    {type->getPointerTo(), i32},
    false
  );
  llvm::Function *func = makeInternalFunc(module, fnType, name);
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  
  llvm::BasicBlock *okBlock = funcBdr.makeBlock();
  llvm::BasicBlock *errorBlock = funcBdr.makeBlock();
  llvm::Value *array = funcBdr.ir.CreateLoad(func->arg_begin());
  llvm::Value *size = funcBdr.ir.CreateLoad(
    funcBdr.ir.CreateStructGEP(array, array_idx_len)
  );
  llvm::Value *inBounds = checkBounds(funcBdr.ir, func->arg_begin() + 1, size);
  likely(funcBdr.ir.CreateCondBr(inBounds, okBlock, errorBlock));
  
  funcBdr.setCurr(okBlock);
  llvm::Value *idx = funcBdr.ir.CreateIntCast(func->arg_begin() + 1, sizeTy, false);
  llvm::Value *data = funcBdr.ir.CreateLoad(
    funcBdr.ir.CreateStructGEP(array, array_idx_dat)
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

llvm::Function *stela::genArrIdxS(
  gen::FuncInst &inst, llvm::Module *module, ast::ArrayType *arr
) {
  return generateArrayIdx(inst, module, arr, checkSignedBounds, "arr_idx_s");
}

llvm::Function *stela::genArrIdxU(
  gen::FuncInst &inst, llvm::Module *module, ast::ArrayType *arr
) {
  return generateArrayIdx(inst, module, arr, checkUnsignedBounds, "arr_idx_u");
}

llvm::Function *stela::genArrLenCtor(
  gen::FuncInst &inst, llvm::Module *module, ast::ArrayType *arr
) {
  llvm::LLVMContext &ctx = module->getContext();
  TypeBuilder typeBdr{ctx};
  llvm::Type *type = generateType(ctx, arr);
  llvm::Type *arrayStructType = type->getPointerElementType();
  llvm::Type *elemPtr = arrayStructType->getStructElementType(array_idx_dat);
  llvm::Type *elem = elemPtr->getPointerElementType();
  llvm::FunctionType *sig = llvm::FunctionType::get(
    elemPtr,
    {type->getPointerTo(), typeBdr.len()},
    false
  );
  llvm::Function *func = makeInternalFunc(module, sig, "arr_len_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  llvm::Value *array = ptrDefCtor(inst, funcBdr, arrayStructType);
  llvm::Value *size = func->arg_begin() + 1;
  
  /*
  ref = 1
  cap = size
  len = size
  dat = malloc(size)
  */
  
  llvm::Value *cap = funcBdr.ir.CreateStructGEP(array, array_idx_cap);
  funcBdr.ir.CreateStore(size, cap);
  llvm::Value *len = funcBdr.ir.CreateStructGEP(array, array_idx_len);
  funcBdr.ir.CreateStore(size, len);
  llvm::Value *dat = funcBdr.ir.CreateStructGEP(array, array_idx_dat);
  llvm::Value *allocation = callAlloc(funcBdr.ir, inst.alloc(), elem, size);
  funcBdr.ir.CreateStore(allocation, dat);
  funcBdr.ir.CreateStore(array, func->arg_begin());
  
  funcBdr.ir.CreateRet(allocation);
  return func;
}

llvm::Function *stela::genArrStrgDtor(
  gen::FuncInst &inst, llvm::Module *module, ast::ArrayType *arr
) {
  llvm::LLVMContext &ctx = module->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    llvm::Type::getVoidTy(ctx),
    {type},
    false
  );
  llvm::Function *func = makeInternalFunc(module, sig, "arr_strg_dtor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  
  llvm::Type *storageStruct = type->getPointerElementType();
  llvm::Type *elemPtr = storageStruct->getStructElementType(array_idx_dat);
  llvm::Type *elemType = elemPtr->getPointerElementType();
  llvm::BasicBlock *head = funcBdr.makeBlock();
  llvm::BasicBlock *body = funcBdr.makeBlock();
  llvm::BasicBlock *done = funcBdr.makeBlock();
  llvm::Value *storage = func->arg_begin();
  llvm::Value *lenPtr = funcBdr.ir.CreateStructGEP(storage, array_idx_len);
  llvm::Value *len = funcBdr.ir.CreateLoad(lenPtr);
  llvm::Value *idxPtr = funcBdr.allocStore(constantFor(len, 0));
  llvm::Value *datPtr = funcBdr.ir.CreateStructGEP(storage, array_idx_dat);
  llvm::Value *dat = funcBdr.ir.CreateLoad(datPtr);
  
  funcBdr.ir.CreateBr(head);
  funcBdr.setCurr(head);
  llvm::Value *idx = funcBdr.ir.CreateLoad(idxPtr);
  llvm::Value *atEnd = funcBdr.ir.CreateICmpEQ(idx, len);
  llvm::Value *incIdx = funcBdr.ir.CreateNSWAdd(idx, constantFor(idx, 1));
  funcBdr.ir.CreateStore(incIdx, idxPtr);
  funcBdr.ir.CreateCondBr(atEnd, done, body);
  
  funcBdr.setCurr(body);
  llvm::Value *idx1 = funcBdr.ir.CreateLoad(idxPtr);
  llvm::Value *elem = funcBdr.ir.CreateInBoundsGEP(elemType, dat, idx1);
  LifetimeExpr lifetime{inst, funcBdr.ir};
  lifetime.destroy(arr->elem.get(), elem);
  funcBdr.ir.CreateBr(head);
  
  funcBdr.setCurr(done);
  funcBdr.ir.CreateRetVoid();
  return func;
}

namespace {

llvm::Function *unarySrt(
  gen::FuncInst &inst,
  llvm::Module *module,
  ast::StructType *srt,
  const llvm::Twine &name,
  void (LifetimeExpr::*memFun)(ast::Type *, llvm::Value *)
) {
  llvm::Type *type = generateType(module->getContext(), srt);
  llvm::Function *func = makeInternalFunc(module, unaryCtorFor(type), name);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder funcBdr{func};
  LifetimeExpr lifetime{inst, funcBdr.ir};
  
  const unsigned members = type->getNumContainedTypes();
  for (unsigned m = 0; m != members; ++m) {
    llvm::Value *memPtr = funcBdr.ir.CreateStructGEP(func->arg_begin(), m);
    (lifetime.*memFun)(srt->fields[m].type.get(), memPtr);
  }
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *binarySrt(
  gen::FuncInst &inst,
  llvm::Module *module,
  ast::StructType *srt,
  const llvm::Twine &name,
  void (LifetimeExpr::*memFun)(ast::Type *, llvm::Value *, llvm::Value *)
) {
  llvm::Type *type = generateType(module->getContext(), srt);
  llvm::Function *func = makeInternalFunc(module, binaryCtorFor(type), name);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  if (memFun != &LifetimeExpr::copyAssign) {
    func->addParamAttr(0, llvm::Attribute::NoAlias);
    func->addParamAttr(1, llvm::Attribute::NoAlias);
  }
  FuncBuilder funcBdr{func};
  LifetimeExpr lifetime{inst, funcBdr.ir};
  
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

llvm::Function *stela::genSrtDtor(
  gen::FuncInst &inst, llvm::Module *module, ast::StructType *srt
) {
  return unarySrt(inst, module, srt, "srt_dtor", &LifetimeExpr::destroy);
}

llvm::Function *stela::genSrtDefCtor(
  gen::FuncInst &inst, llvm::Module *module, ast::StructType *srt
) {
  return unarySrt(inst, module, srt, "srt_def_ctor", &LifetimeExpr::defConstruct);
}

llvm::Function *stela::genSrtCopCtor(
  gen::FuncInst &inst, llvm::Module *module, ast::StructType *srt
) {
  return binarySrt(inst, module, srt, "srt_cop_ctor", &LifetimeExpr::copyConstruct);
}

llvm::Function *stela::genSrtCopAsgn(
  gen::FuncInst &inst, llvm::Module *module, ast::StructType *srt
) {
  return binarySrt(inst, module, srt, "srt_cop_asgn", &LifetimeExpr::copyAssign);
}

llvm::Function *stela::genSrtMovCtor(
  gen::FuncInst &inst, llvm::Module *module, ast::StructType *srt
) {
  return binarySrt(inst, module, srt, "srt_mov_ctor", &LifetimeExpr::moveConstruct);
}

llvm::Function *stela::genSrtMovAsgn(
  gen::FuncInst &inst, llvm::Module *module, ast::StructType *srt
) {
  return binarySrt(inst, module, srt, "srt_mov_asgn", &LifetimeExpr::moveAssign);
}

namespace {

gen::Expr lvalue(llvm::Value *obj) {
  return {obj, ValueCat::lvalue};
}

}

llvm::Function *stela::genSrtEq(
  gen::FuncInst &inst, llvm::Module *module, ast::StructType *srt
) {
  llvm::Type *type = generateType(module->getContext(), srt);
  llvm::Function *func = makeInternalFunc(module, compareFor(type), "srt_eq");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::ReadOnly);
  FuncBuilder funcBdr{func};
  CompareExpr compare{inst, funcBdr.ir};
  llvm::BasicBlock *diffBlock = funcBdr.makeBlock();
  
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
  
  funcBdr.ir.CreateRet(funcBdr.ir.getInt1(true));
  funcBdr.setCurr(diffBlock);
  funcBdr.ir.CreateRet(funcBdr.ir.getInt1(false));
  
  return func;
}

llvm::Function *stela::genSrtLt(
  gen::FuncInst &inst, llvm::Module *module, ast::StructType *srt
) {
  llvm::Type *type = generateType(module->getContext(), srt);
  llvm::Function *func = makeInternalFunc(module, compareFor(type), "srt_lt");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::ReadOnly);
  FuncBuilder funcBdr{func};
  CompareExpr compare{inst, funcBdr.ir};
  llvm::BasicBlock *ltBlock = funcBdr.makeBlock();
  llvm::BasicBlock *geBlock = funcBdr.makeBlock();
  
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
  funcBdr.ir.CreateRet(funcBdr.ir.getInt1(true));
  funcBdr.setCurr(geBlock);
  funcBdr.ir.CreateRet(funcBdr.ir.getInt1(false));
  
  return func;
}
