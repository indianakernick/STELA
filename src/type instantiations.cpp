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
}

namespace {

/*
bool notInst(std::unordered_set<gen::String> &set, const gen::String &string) {
  // second is true if string was inserted
  // if string was inserted, type was not instantiated
  return set.insert(string.dup()).second;
}
*/

}

llvm::Function *gen::TypeInst::array(llvm::Type *type) {
  auto iter = arrayDtors.find(type);
  if (iter == arrayDtors.end()) {
    llvm::Function *dtor = generateArrayDtor(module, type);
    arrayDtors.insert({type, dtor});
    return dtor;
  } else {
    return iter->second;
  }
}
