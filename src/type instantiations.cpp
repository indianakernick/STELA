//
//  type instantiations.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "type instantiations.hpp"

#include "generate func.hpp"

using namespace stela;

gen::TypeInst::TypeInst(llvm::Module *module)
  : module{module} {
  arrayDtors.reserve(32);
  arrayDefCtors.reserve(32);
}

llvm::Function *gen::TypeInst::arrayDtor(llvm::Type *type) {
  return getCached(arrayDtors, generateArrayDtor, type);
}

llvm::Function *gen::TypeInst::arrayDefCtor(llvm::Type *type) {
  return getCached(arrayDefCtors, generateArrayDefCtor, type);
}

llvm::Function *gen::TypeInst::arrayCopCtor(llvm::Type *type) {
  return getCached(arrayCopCtors, generateArrayCopCtor, type);
}

llvm::Function *gen::TypeInst::arrayCopAsgn(llvm::Type *type) {
  return getCached(arrayCopAsgns, generateArrayCopAsgn, type);
}

llvm::Function *gen::TypeInst::getCached(FuncMap &map, MakeFunc *make, llvm::Type *type) {
  const auto iter = map.find(type);
  if (iter == map.end()) {
    llvm::Function *func = make(module, type);
    map.insert({type, func});
    return func;
  } else {
    return iter->second;
  }
}
