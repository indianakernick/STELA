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

}

namespace stela {

llvm::ExecutionEngine *generate(const Symbols &, LogSink &);

}

#endif
