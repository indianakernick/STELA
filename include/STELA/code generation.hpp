//
//  code generation.hpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_code_generation_hpp
#define stela_code_generation_hpp

#include "log.hpp"
#include "symbols.hpp"

namespace llvm {

class ExecutionEngine;
class Module;

}

namespace stela {

struct OptFlags {
  bool inliner = true;
  bool vectorize = true;
  bool optimizeIR = true;
  bool optimizeASM = true;
};

constexpr OptFlags opt_all = {};
constexpr OptFlags opt_none = {false, false, false, false};

std::unique_ptr<llvm::Module> generateIR(const Symbols &, LogSink &);
llvm::ExecutionEngine *generateCode(std::unique_ptr<llvm::Module>, LogSink &, OptFlags = opt_all);
llvm::ExecutionEngine *generateCode(const Symbols &, LogSink &, OptFlags = opt_all);

}

#endif
