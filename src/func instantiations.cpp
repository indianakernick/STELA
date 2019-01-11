//
//  func instantiations.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "func instantiations.hpp"

#include <llvm/IR/Module.h>
#include "generate type.hpp"
#include "generate func.hpp"
#include "generate array.hpp"
#include "generate struct.hpp"

using namespace stela;

FuncInst::FuncInst(llvm::Module *module)
  : module{module} {}

llvm::Function *FuncInst::arrayDtor(ast::ArrayType *type) {
  return getCached(arrayDtors, genArrDtor, type);
}

llvm::Function *FuncInst::arrayDefCtor(ast::ArrayType *type) {
  return getCached(arrayDefCtors, genArrDefCtor, type);
}

llvm::Function *FuncInst::arrayCopCtor(ast::ArrayType *type) {
  return getCached(arrayCopCtors, genArrCopCtor, type);
}

llvm::Function *FuncInst::arrayCopAsgn(ast::ArrayType *type) {
  return getCached(arrayCopAsgns, genArrCopAsgn, type);
}

llvm::Function *FuncInst::arrayMovCtor(ast::ArrayType *type) {
  return getCached(arrayMovCtors, genArrMovCtor, type);
}

llvm::Function *FuncInst::arrayMovAsgn(ast::ArrayType *type) {
  return getCached(arrayMovAsgns, genArrMovAsgn, type);
}

llvm::Function *FuncInst::arrayIdxS(ast::ArrayType *type) {
  return getCached(arrayIdxSs, genArrIdxS, type);
}

llvm::Function *FuncInst::arrayIdxU(ast::ArrayType *type) {
  return getCached(arrayIdxUs, genArrIdxU, type);
}

llvm::Function *FuncInst::arrayLenCtor(ast::ArrayType *type) {
  return getCached(arrayLenCtors, genArrLenCtor, type);
}

llvm::Function *FuncInst::arrayStrgDtor(ast::ArrayType *type) {
  return getCached(arrayStrgDtors, genArrStrgDtor, type);
}

llvm::Function *FuncInst::arrayEq(ast::ArrayType *type) {
  return getCached(arrayEqs, genArrEq, type);
}

llvm::Function *FuncInst::arrayLt(ast::ArrayType *type) {
  return getCached(arrayLts, genArrLt, type);
}

llvm::Function *FuncInst::structDtor(ast::StructType *type) {
  return getCached(structDtors, genSrtDtor, type);
}

llvm::Function *FuncInst::structDefCtor(ast::StructType *type) {
  return getCached(structDefCtors, genSrtDefCtor, type);
}

llvm::Function *FuncInst::structCopCtor(ast::StructType *type) {
  return getCached(structCopCtors, genSrtCopCtor, type);
}

llvm::Function *FuncInst::structCopAsgn(ast::StructType *type) {
  return getCached(structCopAsgns, genSrtCopAsgn, type);
}

llvm::Function *FuncInst::structMovCtor(ast::StructType *type) {
  return getCached(structMovCtors, genSrtMovCtor, type);
}

llvm::Function *FuncInst::structMovAsgn(ast::StructType *type) {
  return getCached(structMovAsgns, genSrtMovAsgn, type);
}

llvm::Function *FuncInst::structEq(ast::StructType *type) {
  return getCached(structEqs, genSrtEq, type);
}

llvm::Function *FuncInst::structLt(ast::StructType *type) {
  return getCached(structLts, genSrtLt, type);
}

llvm::Function *FuncInst::panic() {
  return getCached(panicFn, genPanic);
}

llvm::Function *FuncInst::alloc() {
  return getCached(allocFn, genAlloc);
}

llvm::Function *FuncInst::free() {
  return getCached(freeFn, genFree);
}

llvm::Function *FuncInst::ceilToPow2() {
  return getCached(ceilToPow2Fn, genCeilToPow2);
}

template <typename Make, typename Type>
llvm::Function *FuncInst::getCached(FuncMap &map, Make *make, Type *ast) {
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

llvm::Function *FuncInst::getCached(
  llvm::Function *&func,
  llvm::Function *(*make)(InstData)
) {
  if (!func) {
    func = make({*this, module});
  }
  return func;
}
