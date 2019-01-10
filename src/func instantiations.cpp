//
//  func instantiations.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "func instantiations.hpp"

#include <llvm/IR/Module.h>
#include "generate func.hpp"
#include "generate type.hpp"

using namespace stela;

gen::FuncInst::FuncInst(llvm::Module *module)
  : module{module} {}

llvm::Function *gen::FuncInst::arrayDtor(ast::ArrayType *type) {
  return getCached(arrayDtors, genArrDtor, type);
}

llvm::Function *gen::FuncInst::arrayDefCtor(ast::ArrayType *type) {
  return getCached(arrayDefCtors, genArrDefCtor, type);
}

llvm::Function *gen::FuncInst::arrayCopCtor(ast::ArrayType *type) {
  return getCached(arrayCopCtors, genArrCopCtor, type);
}

llvm::Function *gen::FuncInst::arrayCopAsgn(ast::ArrayType *type) {
  return getCached(arrayCopAsgns, genArrCopAsgn, type);
}

llvm::Function *gen::FuncInst::arrayMovCtor(ast::ArrayType *type) {
  return getCached(arrayMovCtors, genArrMovCtor, type);
}

llvm::Function *gen::FuncInst::arrayMovAsgn(ast::ArrayType *type) {
  return getCached(arrayMovAsgns, genArrMovAsgn, type);
}

llvm::Function *gen::FuncInst::arrayIdxS(ast::ArrayType *type) {
  return getCached(arrayIdxSs, genArrIdxS, type);
}

llvm::Function *gen::FuncInst::arrayIdxU(ast::ArrayType *type) {
  return getCached(arrayIdxUs, genArrIdxU, type);
}

llvm::Function *gen::FuncInst::arrayLenCtor(ast::ArrayType *type) {
  return getCached(arrayLenCtors, genArrLenCtor, type);
}

llvm::Function *gen::FuncInst::arrayStrgDtor(ast::ArrayType *type) {
  return getCached(arrayStoreDtors, genArrStrgDtor, type);
}

llvm::Function *gen::FuncInst::structDtor(ast::StructType *type) {
  return getCached(structDtors, genSrtDtor, type);
}

llvm::Function *gen::FuncInst::structDefCtor(ast::StructType *type) {
  return getCached(structDefCtors, genSrtDefCtor, type);
}

llvm::Function *gen::FuncInst::structCopCtor(ast::StructType *type) {
  return getCached(structCopCtors, genSrtCopCtor, type);
}

llvm::Function *gen::FuncInst::structCopAsgn(ast::StructType *type) {
  return getCached(structCopAsgns, genSrtCopAsgn, type);
}

llvm::Function *gen::FuncInst::structMovCtor(ast::StructType *type) {
  return getCached(structMovCtors, genSrtMovCtor, type);
}

llvm::Function *gen::FuncInst::structMovAsgn(ast::StructType *type) {
  return getCached(structMovAsgns, genSrtMovAsgn, type);
}

llvm::Function *gen::FuncInst::structEq(ast::StructType *type) {
  return getCached(structEqs, genSrtEq, type);
}

llvm::Function *gen::FuncInst::structLt(ast::StructType *type) {
  return getCached(structLts, genSrtLt, type);
}

llvm::Function *gen::FuncInst::panic() {
  if (!panicFn) {
    panicFn = generatePanic({*this, module});
  }
  return panicFn;
}

llvm::Function *gen::FuncInst::alloc() {
  if (!allocFn) {
    allocFn = generateAlloc({*this, module});
  }
  return allocFn;
}

llvm::Function *gen::FuncInst::free() {
  if (!freeFn) {
    freeFn = generateFree({*this, module});
  }
  return freeFn;
}

template <typename Make, typename Type>
llvm::Function *gen::FuncInst::getCached(FuncMap &map, Make *make, Type *ast) {
  llvm::Type *type = generateType(module->getContext(), ast);
  const auto iter = map.find(type);
  if (iter == map.end()) {
    llvm::Function *func = make({*this, module}, ast);
    map.insert({type, func});
    return func;
  } else {
    return iter->second;
  }
}
