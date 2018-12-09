//
//  semantic analysis.hpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_semantic_analysis_hpp
#define stela_semantic_analysis_hpp

#include "log.hpp"
#include "ast.hpp"
#include "symbols.hpp"
#include "modules.hpp"

namespace stela {

/// Initialize the builtin and standard library modules
Symbols initModules(LogSink &);
/// Perform semantic analysis on an AST and create a module. The declarations
/// are moved out of the AST object and into Symbols. AST object can be
/// discarded
void compileModule(Symbols &, AST &, LogSink &);
/// Compile the ASTs into Modules in the right order
void compileModules(Symbols &, const ModuleOrder &, ASTs &, LogSink &);

}

#endif
