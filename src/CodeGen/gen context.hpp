//
//  gen context.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_gen_context_hpp
#define stela_gen_context_hpp

#include "Log/log output.hpp"
#include "func instantiations.hpp"

namespace llvm {

class LLVMContext;
class Module;

}

namespace stela::gen {

struct Ctx {
  llvm::LLVMContext &llvm;
  llvm::Module *mod;
  FuncInst &inst;
  Log &log;
};

}

#endif
