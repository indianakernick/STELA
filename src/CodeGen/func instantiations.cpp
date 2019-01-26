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

using namespace stela;

FuncInst::FuncInst(llvm::Module *module)
  : module{module} {
  fns.fill(nullptr);
  for (FuncMap &map : paramFns) {
    map.reserve(16);
  }
}

llvm::Type *FuncInst::getType(ast::Type *type) {
  return generateType(module->getContext(), type);
}
