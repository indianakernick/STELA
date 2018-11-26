//
//  context.hpp
//  STELA
//
//  Created by Indi Kernick on 18/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_context_hpp
#define stela_context_hpp

#include "symbols.hpp"
#include "log output.hpp"
#include "scope manager.hpp"

namespace stela::sym {

struct Ctx {
  const sym::Builtins &btn;
  ScopeMan &man;
  Log &log;
};

}

#endif
