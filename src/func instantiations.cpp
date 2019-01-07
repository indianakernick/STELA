//
//  func instantiations.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "func instantiations.hpp"

#include "generate func.hpp"

using namespace stela;

gen::FuncInst::FuncInst(llvm::Module *module)
  : module{module} {
  arrayDtors.reserve(32);
  arrayDefCtors.reserve(32);
}

llvm::Function *gen::FuncInst::arrayDtor(llvm::Type *type) {
  return getCached(arrayDtors, generateArrayDtor, type);
}

llvm::Function *gen::FuncInst::arrayDefCtor(llvm::Type *type) {
  return getCached(arrayDefCtors, generateArrayDefCtor, type);
}

llvm::Function *gen::FuncInst::arrayCopCtor(llvm::Type *type) {
  return getCached(arrayCopCtors, generateArrayCopCtor, type);
}

llvm::Function *gen::FuncInst::arrayCopAsgn(llvm::Type *type) {
  return getCached(arrayCopAsgns, generateArrayCopAsgn, type);
}

llvm::Function *gen::FuncInst::arrayMovCtor(llvm::Type *type) {
  return getCached(arrayMovCtors, generateArrayMovCtor, type);
}

llvm::Function *gen::FuncInst::arrayMovAsgn(llvm::Type *type) {
  return getCached(arrayMovAsgns, generateArrayMovAsgn, type);
}

llvm::Function *gen::FuncInst::arrayIdxS(llvm::Type *type) {
  return getCached(arrayIdxSs, generateArrayIdxS, type);
}

llvm::Function *gen::FuncInst::arrayIdxU(llvm::Type *type) {
  return getCached(arrayIdxUs, generateArrayIdxU, type);
}

llvm::Function *gen::FuncInst::structDtor(llvm::Type *type, ast::StructType *srt) {
  return getCached(structDtors, genSrtDtor, type, srt);
}

llvm::Function *gen::FuncInst::structDefCtor(llvm::Type *type, ast::StructType *srt) {
  return getCached(structDefCtors, genSrtDefCtor, type, srt);
}

llvm::Function *gen::FuncInst::structCopCtor(llvm::Type *type, ast::StructType *srt) {
  return getCached(structCopCtors, genSrtCopCtor, type, srt);
}

llvm::Function *gen::FuncInst::structCopAsgn(llvm::Type *type, ast::StructType *srt) {
  return getCached(structCopAsgns, genSrtCopAsgn, type, srt);
}

llvm::Function *gen::FuncInst::structMovCtor(llvm::Type *type, ast::StructType *srt) {
  return getCached(structMovCtors, genSrtMovCtor, type, srt);
}

llvm::Function *gen::FuncInst::structMovAsgn(llvm::Type *type, ast::StructType *srt) {
  return getCached(structMovAsgns, genSrtMovAsgn, type, srt);
}

llvm::Function *gen::FuncInst::panic() {
  if (!panicFn) {
    panicFn = generatePanic(module);
  }
  return panicFn;
}

llvm::Function *gen::FuncInst::alloc() {
  if (!allocFn) {
    allocFn = generateAlloc(*this, module);
  }
  return allocFn;
}

llvm::Function *gen::FuncInst::free() {
  if (!freeFn) {
    freeFn = generateFree(module);
  }
  return freeFn;
}

llvm::Function *gen::FuncInst::getCached(FuncMap &map, MakeArray *make, llvm::Type *type) {
  const auto iter = map.find(type);
  if (iter == map.end()) {
    llvm::Function *func = make(*this, module, type);
    map.insert({type, func});
    return func;
  } else {
    return iter->second;
  }
}

llvm::Function *gen::FuncInst::getCached(FuncMap &map, MakeStruct *make, llvm::Type *type, ast::StructType *srt) {
  const auto iter = map.find(type);
  if (iter == map.end()) {
    llvm::Function *func = make(*this, module, type, srt);
    map.insert({type, func});
    return func;
  } else {
    return iter->second;
  }
}
