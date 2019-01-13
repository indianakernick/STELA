//
//  generate closure.cpp
//  STELA
//
//  Created by Indi Kernick on 13/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "gen helpers.hpp"
#include "generate type.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

namespace {

void stubAttributes(llvm::Function *func) {
  const size_t args = func->arg_size();
  for (unsigned a = 0; a != args; ++a) {
    if (func->hasParamAttribute(a, llvm::Attribute::ReadOnly)) {
      func->removeParamAttr(a, llvm::Attribute::ReadOnly);
      func->addParamAttr(a, llvm::Attribute::ReadNone);
    }
    if (func->hasParamAttribute(a, llvm::Attribute::WriteOnly)) {
      func->removeParamAttr(a, llvm::Attribute::WriteOnly);
      func->addParamAttr(a, llvm::Attribute::ReadNone);
    }
  }
  func->addFnAttr(llvm::Attribute::NoReturn);
}

}

template <>
llvm::Function *stela::genFn<PFGI::clo_stub>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  const Signature sig = getSignature(*clo);
  llvm::FunctionType *fnType = generateSig(ctx, sig);
  llvm::Function *func = makeInternalFunc(data.mod, fnType, "clo_stub");
  assignAttrs(func, sig);
  stubAttributes(func);
  FuncBuilder builder{func};
  llvm::Function *panic = data.inst.get<FGI::panic>();
  callPanic(builder.ir, panic, "Calling null function pointer");
  return func;
}
