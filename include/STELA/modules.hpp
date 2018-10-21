//
//  modules.hpp
//  STELA
//
//  Created by Indi Kernick on 21/10/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_modules_hpp
#define stela_modules_hpp

#include "ast.hpp"
#include "log.hpp"

namespace stela {

using ModuleOrder = std::vector<size_t>;

/// Find the order which modules must be compiled in. Cyclic dependencies are
/// not allowed
ModuleOrder findModuleOrder(const ASTs &, LogBuf &);

}

#endif
