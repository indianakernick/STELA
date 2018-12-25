//
//  gen context.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_gen_context_hpp
#define stela_gen_context_hpp

#include "log output.hpp"
#include "func instantiations.hpp"

namespace llvm {

class LLVMContext;

}

namespace stela::gen {

struct Ctx {
  llvm::LLVMContext &llvm;
  FuncInst &inst;
  Log &log;
};

}

#endif
