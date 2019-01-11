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

// constexpr unsigned array_idx_ref = 0;
constexpr unsigned array_idx_cap = 1;
constexpr unsigned array_idx_len = 2;
constexpr unsigned array_idx_dat = 3;

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

void assignUnaryCtorAttrs(llvm::Function *func) {
  func->addFnAttr(llvm::Attribute::NoRecurse);
  func->addParamAttr(0, llvm::Attribute::NonNull);
}

void assignBinaryAliasCtorAttrs(llvm::Function *func) {
  func->addFnAttr(llvm::Attribute::NoRecurse);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
}

void assignBinaryCtorAttrs(llvm::Function *func) {
  assignBinaryAliasCtorAttrs(func);
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(1, llvm::Attribute::NoAlias);
}

void assignCompareAttrs(llvm::Function *func) {
  assignBinaryAliasCtorAttrs(func);
  func->addAttribute(0, llvm::Attribute::ZExt);
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(1, llvm::Attribute::ReadOnly);
  func->addFnAttr(llvm::Attribute::ReadOnly);
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

llvm::Value *loadStructElem(llvm::IRBuilder<> &ir, llvm::Value *srtPtr, unsigned idx) {
  return ir.CreateLoad(ir.CreateStructGEP(srtPtr, idx));
}

llvm::Value *arrayIndex(llvm::IRBuilder<> &ir, llvm::Value *ptr, llvm::Value *idx) {
  llvm::Type *sizeTy = getType<size_t>(ir.getContext());
  llvm::Value *wideIdx = ir.CreateIntCast(idx, sizeTy, false);
  llvm::Type *elemTy = ptr->getType()->getPointerElementType();
  return ir.CreateInBoundsGEP(elemTy, ptr, wideIdx);
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
  return callAlloc(ir, alloc, type, constantFor(sizeTy, 1));
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

enum class RefChg {
  inc,
  dec
};

llvm::Value *getRef(llvm::IRBuilder<> &ir, llvm::Value *ptr) {
  return ir.CreateStructGEP(ptr, 0);
}

llvm::Value *refChange(llvm::IRBuilder<> &ir, llvm::Value *ptr, const RefChg chg) {
  llvm::Value *ref = getRef(ir, ptr);
  llvm::Value *refVal = ir.CreateLoad(ref);
  llvm::Constant *one = constantFor(refVal, 1);
  llvm::Value *changed = chg == RefChg::inc ? ir.CreateNSWAdd(refVal, one)
                                            : ir.CreateNSWSub(refVal, one);
  ir.CreateStore(changed, ref);
  return changed;
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
  llvm::Value *subed = refChange(funcBdr.ir, ptr, RefChg::dec);
  llvm::Value *isZero = funcBdr.ir.CreateICmpEQ(subed, constantFor(subed, 0));
  funcBdr.ir.CreateCondBr(isZero, del, done);
  funcBdr.setCurr(del);
  funcBdr.ir.CreateCall(dtor, {ptr});
  callFree(funcBdr.ir, inst.free(), ptr);
  funcBdr.branch(done);
}

llvm::Value *ptrDefCtor(
  gen::FuncInst &inst, FuncBuilder &funcBdr, llvm::Type *elemType
) {
  /*
  ptr = malloc elemType
  ptr.ref = 1
  */
  llvm::Value *ptr = callAlloc(funcBdr.ir, inst.alloc(), elemType);
  llvm::Value *ref = getRef(funcBdr.ir, ptr);
  funcBdr.ir.CreateStore(constantForPtr(ref, 1), ref);
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
  refChange(funcBdr.ir, other, RefChg::inc);
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
  refChange(funcBdr.ir, right, RefChg::inc);
  llvm::Value *left = funcBdr.ir.CreateLoad(leftPtr);
  resetPtr(inst, funcBdr, left, dtor, innerDone);
  funcBdr.ir.CreateStore(right, leftPtr);
  funcBdr.branch(done);
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
  funcBdr.branch(done);
}

void likely(llvm::BranchInst *branch) {
  llvm::LLVMContext &ctx = branch->getContext();
  llvm::IntegerType *i32 = llvm::IntegerType::getInt32Ty(ctx);
  llvm::Metadata *troo = llvm::ConstantAsMetadata::get(constantFor(i32, 2048));
  llvm::Metadata *fols = llvm::ConstantAsMetadata::get(constantFor(i32, 1));
  llvm::MDTuple *tuple = llvm::MDNode::get(ctx, {
    llvm::MDString::get(ctx, "branch_weights"), troo, fols
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

llvm::Function *stela::generatePanic(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::Type *voidTy = llvm::Type::getVoidTy(ctx);
  llvm::Type *strTy = getTypePtr<char>(ctx);
  llvm::Type *intTy = getType<int>(ctx);
  llvm::FunctionType *putsType = llvm::FunctionType::get(intTy, {strTy}, false);
  llvm::FunctionType *exitType = llvm::FunctionType::get(voidTy, {intTy}, false);
  llvm::FunctionType *panicType = llvm::FunctionType::get(voidTy, {strTy}, false);
  
  llvm::Function *puts = declareCFunc(data.mod, putsType, "puts");
  llvm::Function *exit = declareCFunc(data.mod, exitType, "exit");
  llvm::Function *panic = makeInternalFunc(data.mod, panicType, "panic");
  puts->addParamAttr(0, llvm::Attribute::ReadOnly);
  puts->addParamAttr(0, llvm::Attribute::NoCapture);
  exit->addFnAttr(llvm::Attribute::NoReturn);
  exit->addFnAttr(llvm::Attribute::Cold);
  panic->addFnAttr(llvm::Attribute::NoReturn);
  
  FuncBuilder funcBdr{panic};
  funcBdr.ir.CreateCall(puts, panic->arg_begin());
  funcBdr.ir.CreateCall(exit, constantFor(intTy, EXIT_FAILURE));
  funcBdr.ir.CreateUnreachable();
  
  return panic;
}

llvm::Function *stela::generateAlloc(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::Type *memTy = llvm::Type::getInt8PtrTy(ctx);
  llvm::Type *sizeTy = getType<size_t>(ctx);
  llvm::FunctionType *mallocType = llvm::FunctionType::get(memTy, {sizeTy}, false);
  llvm::FunctionType *allocType = mallocType;
  
  llvm::Function *malloc = declareCFunc(data.mod, mallocType, "malloc");
  llvm::Function *alloc = makeInternalFunc(data.mod, allocType, "alloc");
  malloc->addAttribute(0, llvm::Attribute::NoAlias);
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
  callPanic(funcBdr.ir, data.inst.panic(), "Out of memory");
  
  return alloc;
}

llvm::Function *stela::generateFree(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *voidTy = llvm::Type::getVoidTy(ctx);
  llvm::Type *memTy = llvm::Type::getInt8PtrTy(ctx);
  llvm::FunctionType *freeType = llvm::FunctionType::get(voidTy, {memTy}, false);
  llvm::Function *free = declareCFunc(data.mod, freeType, "free");
  free->addParamAttr(0, llvm::Attribute::NoCapture);
  return free;
}

llvm::Function *stela::genCeilToPow2(InstData data) {
  // this returns 2 when the input is 1
  // we could subtract val == 1 but that's just more instructions for no reason!
  llvm::LLVMContext &ctx = data.mod->getContext();
  TypeBuilder typeBdr{ctx};
  llvm::Type *type = typeBdr.len();
  llvm::FunctionType *fnType = llvm::FunctionType::get(
    type, {type}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, fnType, "ceil_to_pow_2");
  FuncBuilder funcBdr{func};
  
  llvm::DataLayout layout{data.mod};
  const uint64_t bitSize = layout.getTypeSizeInBits(type);
  llvm::FunctionType *ctlzType = llvm::FunctionType::get(
    type, {type, funcBdr.ir.getInt1Ty()}, false
  );
  llvm::Twine name = llvm::Twine{"llvm.ctlz.i"} + llvm::Twine{bitSize};
  llvm::Function *ctlz = declareCFunc(data.mod, ctlzType, name);
  llvm::Value *minus1 = funcBdr.ir.CreateNSWSub(func->arg_begin(), constantFor(type, 1));
  llvm::Value *leadingZeros = funcBdr.ir.CreateCall(ctlz, {minus1, funcBdr.ir.getTrue()});
  llvm::Value *bits = constantFor(type, bitSize);
  llvm::Value *log2 = funcBdr.ir.CreateNSWSub(bits, leadingZeros);
  llvm::Value *ceiled = funcBdr.ir.CreateShl(constantFor(type, 1), log2);
  
  funcBdr.ir.CreateRet(ceiled);
  return func;
}

llvm::Function *stela::genArrDtor(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, unaryCtorFor(type), "arr_dtor");
  assignUnaryCtorAttrs(func);
  FuncBuilder funcBdr{func};
  llvm::BasicBlock *done = funcBdr.makeBlock();
  ptrDtor(data.inst, funcBdr, func->arg_begin(), data.inst.arrayStrgDtor(arr), done);
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrDefCtor(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, unaryCtorFor(type), "arr_def_ctor");
  assignUnaryCtorAttrs(func);
  FuncBuilder funcBdr{func};
  llvm::Type *arrayStructType = type->getPointerElementType();
  llvm::Value *array = ptrDefCtor(data.inst, funcBdr, arrayStructType);

  /*
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

llvm::Function *stela::genArrCopCtor(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), "arr_cop_ctor");
  assignBinaryCtorAttrs(func);
  FuncBuilder funcBdr{func};
  ptrCopCtor(funcBdr, func->arg_begin(), func->arg_begin() + 1);
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrCopAsgn(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), "arr_cop_asgn");
  assignBinaryAliasCtorAttrs(func);
  FuncBuilder funcBdr{func};
  llvm::BasicBlock *done = funcBdr.makeBlock();
  ptrCopAsgn(data.inst, funcBdr, func->arg_begin(), func->arg_begin() + 1, data.inst.arrayStrgDtor(arr), done);
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrMovCtor(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), "arr_mov_ctor");
  assignBinaryCtorAttrs(func);
  FuncBuilder funcBdr{func};
  ptrMovCtor(funcBdr, func->arg_begin(), func->arg_begin() + 1);
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genArrMovAsgn(InstData data, ast::ArrayType *arr) {
  llvm::Type *type = generateType(data.mod->getContext(), arr);
  llvm::Function *func = makeInternalFunc(data.mod, binaryCtorFor(type), "arr_mov_asgn");
  assignBinaryCtorAttrs(func);
  FuncBuilder funcBdr{func};
  llvm::BasicBlock *done = funcBdr.makeBlock();
  ptrMovAsgn(data.inst, funcBdr, func->arg_begin(), func->arg_begin() + 1, data.inst.arrayStrgDtor(arr), done);
  funcBdr.ir.CreateRetVoid();
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
  FuncBuilder funcBdr{func};
  
  llvm::BasicBlock *okBlock = funcBdr.makeBlock();
  llvm::BasicBlock *errorBlock = funcBdr.makeBlock();
  llvm::Value *array = funcBdr.ir.CreateLoad(func->arg_begin());
  llvm::Value *len = loadStructElem(funcBdr.ir, array, array_idx_len);
  llvm::Value *inBounds = checkBounds(funcBdr.ir, func->arg_begin() + 1, len);
  likely(funcBdr.ir.CreateCondBr(inBounds, okBlock, errorBlock));
  
  funcBdr.setCurr(okBlock);
  llvm::Value *dat = loadStructElem(funcBdr.ir, array, array_idx_dat);
  funcBdr.ir.CreateRet(arrayIndex(funcBdr.ir, dat, func->arg_begin() + 1));
  
  funcBdr.setCurr(errorBlock);
  callPanic(funcBdr.ir, data.inst.panic(), "Index out of bounds");
  
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

llvm::Function *stela::genArrIdxS(InstData data, ast::ArrayType *arr) {
  return generateArrayIdx(data, arr, checkSignedBounds, "arr_idx_s");
}

llvm::Function *stela::genArrIdxU(InstData data, ast::ArrayType *arr) {
  return generateArrayIdx(data, arr, checkUnsignedBounds, "arr_idx_u");
}

llvm::Function *stela::genArrLenCtor(InstData data, ast::ArrayType *arr) {
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
  FuncBuilder funcBdr{func};
  llvm::Value *array = ptrDefCtor(data.inst, funcBdr, arrayStructType);
  llvm::Value *size = func->arg_begin() + 1;
  
  /*
  cap = size
  len = size
  dat = malloc(size)
  */
  
  llvm::Value *cap = funcBdr.ir.CreateStructGEP(array, array_idx_cap);
  funcBdr.ir.CreateStore(size, cap);
  llvm::Value *len = funcBdr.ir.CreateStructGEP(array, array_idx_len);
  funcBdr.ir.CreateStore(size, len);
  llvm::Value *dat = funcBdr.ir.CreateStructGEP(array, array_idx_dat);
  llvm::Value *allocation = callAlloc(funcBdr.ir, data.inst.alloc(), elem, size);
  funcBdr.ir.CreateStore(allocation, dat);
  funcBdr.ir.CreateStore(array, func->arg_begin());
  
  funcBdr.ir.CreateRet(allocation);
  return func;
}

llvm::Function *stela::genArrStrgDtor(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    llvm::Type::getVoidTy(ctx),
    {type},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "arr_strg_dtor");
  assignUnaryCtorAttrs(func);
  FuncBuilder funcBdr{func};
  
  llvm::BasicBlock *head = funcBdr.makeBlock();
  llvm::BasicBlock *body = funcBdr.makeBlock();
  llvm::BasicBlock *done = funcBdr.makeBlock();
  llvm::Value *storage = func->arg_begin();
  llvm::Value *len = loadStructElem(funcBdr.ir, storage, array_idx_len);
  llvm::Value *idxPtr = funcBdr.allocStore(constantFor(len, 0));
  llvm::Value *dat = loadStructElem(funcBdr.ir, storage, array_idx_dat);
  
  funcBdr.branch(head);
  llvm::Value *idx = funcBdr.ir.CreateLoad(idxPtr);
  llvm::Value *atEnd = funcBdr.ir.CreateICmpEQ(idx, len);
  llvm::Value *incIdx = funcBdr.ir.CreateNSWAdd(idx, constantFor(idx, 1));
  funcBdr.ir.CreateStore(incIdx, idxPtr);
  funcBdr.ir.CreateCondBr(atEnd, done, body);
  
  funcBdr.setCurr(body);
  // @TODO can we just use idx?
  llvm::Value *idx1 = funcBdr.ir.CreateLoad(idxPtr);
  LifetimeExpr lifetime{data.inst, funcBdr.ir};
  lifetime.destroy(arr->elem.get(), arrayIndex(funcBdr.ir, dat, idx1));
  funcBdr.ir.CreateBr(head);
  
  funcBdr.setCurr(done);
  funcBdr.ir.CreateRetVoid();
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
  FuncBuilder &funcBdr,
  llvm::Value *leftArr,
  llvm::Value *rightArr,
  llvm::BasicBlock *done
) {
  llvm::BasicBlock *head = funcBdr.makeBlock();
  llvm::BasicBlock *body = funcBdr.makeBlock();
  llvm::BasicBlock *tail = funcBdr.makeBlock();
  
  llvm::Value *leftDat = loadStructElem(funcBdr.ir, leftArr, array_idx_dat);
  llvm::Value *rightDat = loadStructElem(funcBdr.ir, rightArr, array_idx_dat);
  llvm::Value *leftLen = loadStructElem(funcBdr.ir, leftArr, array_idx_len);
  llvm::Value *rightLen = loadStructElem(funcBdr.ir, rightArr, array_idx_len);
  llvm::Value *leftEnd = arrayIndex(funcBdr.ir, leftDat, leftLen);
  llvm::Value *rightEnd = arrayIndex(funcBdr.ir, rightDat, rightLen);
  llvm::Value *leftPtr = funcBdr.allocStore(leftDat);
  llvm::Value *rightPtr = funcBdr.allocStore(rightDat);
  
  funcBdr.branch(head);
  llvm::Value *left = funcBdr.ir.CreateLoad(leftPtr);
  llvm::Value *right = funcBdr.ir.CreateLoad(rightPtr);
  llvm::Value *rightAtEnd = funcBdr.ir.CreateICmpEQ(right, rightEnd);
  funcBdr.ir.CreateCondBr(rightAtEnd, done, body);
  
  funcBdr.setCurr(tail);
  llvm::Value *leftNext = funcBdr.ir.CreateConstInBoundsGEP1_64(left, 1);
  llvm::Value *rightNext = funcBdr.ir.CreateConstInBoundsGEP1_64(right, 1);
  funcBdr.ir.CreateStore(leftNext, leftPtr);
  funcBdr.ir.CreateStore(rightNext, rightPtr);
  funcBdr.ir.CreateBr(head);
  
  funcBdr.setCurr(body);
  return {tail, left, right, leftEnd, rightEnd};
}

gen::Expr lvalue(llvm::Value *obj) {
  return {obj, ValueCat::lvalue};
}

void returnBool(llvm::IRBuilder<> &ir, const bool ret) {
  ir.CreateRet(ir.getInt1(ret));
}

}

llvm::Function *stela::genArrEq(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::Function *func = makeInternalFunc(data.mod, compareFor(type), "arr_eq");
  assignCompareAttrs(func);
  FuncBuilder funcBdr{func};
  
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

  llvm::BasicBlock *compareBlock = funcBdr.makeBlock();
  llvm::BasicBlock *equalBlock = funcBdr.makeBlock();
  llvm::BasicBlock *diffBlock = funcBdr.makeBlock();
  llvm::Value *leftArr = funcBdr.ir.CreateLoad(func->arg_begin());
  llvm::Value *rightArr = funcBdr.ir.CreateLoad(func->arg_begin() + 1);
  llvm::Value *leftLen = loadStructElem(funcBdr.ir, leftArr, array_idx_len);
  llvm::Value *rightLen = loadStructElem(funcBdr.ir, rightArr, array_idx_len);
  llvm::Value *sameLen = funcBdr.ir.CreateICmpEQ(leftLen, rightLen);
  funcBdr.ir.CreateCondBr(sameLen, compareBlock, diffBlock);
  
  funcBdr.setCurr(compareBlock);
  ArrPairLoop comp = iterArrPair(funcBdr, leftArr, rightArr, equalBlock);
  CompareExpr compare{data.inst, funcBdr.ir};
  llvm::Value *eq = compare.equal(arr->elem.get(), lvalue(comp.left), lvalue(comp.right));
  funcBdr.ir.CreateCondBr(eq, comp.loop, diffBlock);
  
  funcBdr.setCurr(equalBlock);
  returnBool(funcBdr.ir, true);
  funcBdr.setCurr(diffBlock);
  returnBool(funcBdr.ir, false);
  
  return func;
}

llvm::Function *stela::genArrLt(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::Function *func = makeInternalFunc(data.mod, compareFor(type), "arr_lt");
  assignCompareAttrs(func);
  FuncBuilder funcBdr{func};
  
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
  
  llvm::BasicBlock *ltBlock = funcBdr.makeBlock();
  llvm::BasicBlock *geBlock = funcBdr.makeBlock();
  llvm::BasicBlock *notEndBlock = funcBdr.makeBlock();
  llvm::BasicBlock *notLtBlock = funcBdr.makeBlock();
  llvm::Value *leftArr = funcBdr.ir.CreateLoad(func->arg_begin());
  llvm::Value *rightArr = funcBdr.ir.CreateLoad(func->arg_begin() + 1);
  ArrPairLoop comp = iterArrPair(funcBdr, leftArr, rightArr, geBlock);
  
  llvm::Value *leftAtEnd = funcBdr.ir.CreateICmpEQ(comp.left, comp.leftEnd);
  funcBdr.ir.CreateCondBr(leftAtEnd, ltBlock, notEndBlock);
  
  funcBdr.setCurr(notEndBlock);
  CompareExpr compare{data.inst, funcBdr.ir};
  llvm::Value *lt = compare.less(arr->elem.get(), lvalue(comp.left), lvalue(comp.right));
  funcBdr.ir.CreateCondBr(lt, ltBlock, notLtBlock);
  
  funcBdr.setCurr(notLtBlock);
  llvm::Value *gt = compare.less(arr->elem.get(), lvalue(comp.right), lvalue(comp.left));
  funcBdr.ir.CreateCondBr(gt, geBlock, comp.loop);
  
  funcBdr.setCurr(ltBlock);
  returnBool(funcBdr.ir, true);
  funcBdr.setCurr(geBlock);
  returnBool(funcBdr.ir, false);
  
  return func;
}

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
