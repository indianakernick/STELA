//
//  gen helpers.hpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_gen_helpers_hpp
#define stela_gen_helpers_hpp

#include "categories.hpp"
#include <llvm/IR/IRBuilder.h>

namespace stela {

// constexpr unsigned array_idx_ref = 0;
constexpr unsigned array_idx_cap = 1;
constexpr unsigned array_idx_len = 2;
constexpr unsigned array_idx_dat = 3;

enum class Inline {
  never,
  smart,
  hint,
  always
};

llvm::Function *makeInternalFunc(
  llvm::Module *,
  llvm::FunctionType *,
  const llvm::Twine &,
  Inline = Inline::always
);
llvm::Function *declareCFunc(llvm::Module *, llvm::FunctionType *, const llvm::Twine &);

llvm::FunctionType *unaryCtorFor(llvm::Type *);
llvm::FunctionType *binaryCtorFor(llvm::Type *);
llvm::FunctionType *compareFor(llvm::Type *);

void assignUnaryCtorAttrs(llvm::Function *);
void assignBinaryAliasCtorAttrs(llvm::Function *);
void assignBinaryCtorAttrs(llvm::Function *);
void assignCompareAttrs(llvm::Function *);

llvm::Constant *constantFor(llvm::Type *, uint64_t);
llvm::Constant *constantFor(llvm::Value *, uint64_t);
llvm::Constant *constantForPtr(llvm::Value *, uint64_t);

llvm::Value *loadStructElem(llvm::IRBuilder<> &, llvm::Value *, unsigned);
llvm::Value *arrayIndex(llvm::IRBuilder<> &, llvm::Value *, llvm::Value *);
void setNull(llvm::IRBuilder<> &, llvm::Value *);
void likely(llvm::BranchInst *);

void callPanic(llvm::IRBuilder<> &, llvm::Function *, std::string_view);
llvm::Value *callAlloc(llvm::IRBuilder<> &, llvm::Function *, llvm::Type *, llvm::Value *);
llvm::Value *callAlloc(llvm::IRBuilder<> &, llvm::Function *, llvm::Type *);
void callFree(llvm::IRBuilder<> &, llvm::Function *, llvm::Value *);

gen::Expr lvalue(llvm::Value *);
void returnBool(llvm::IRBuilder<> &, bool);

llvm::PointerType *refPtrTy(llvm::LLVMContext &);
llvm::PointerType *refPtrPtrTy(llvm::LLVMContext &);
llvm::Value *refPtrPtrCast(llvm::IRBuilder<> &, llvm::Value *);
void initRefCount(llvm::IRBuilder<> &, llvm::Value *);

}

#endif
