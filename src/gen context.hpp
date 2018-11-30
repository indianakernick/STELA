//
//  gen context.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_gen_context_hpp
#define stela_gen_context_hpp

#include "gen string.hpp"
#include "log output.hpp"
#include "type instantiations.hpp"

namespace stela::gen {

struct Ctx {
  String &type;
  String &fun;
  TypeInst &inst;
  Log &log;
};

}

#endif
