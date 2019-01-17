//
//  generate stat.hpp
//  STELA
//
//  Created by Indi Kernick on 10/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_stat_hpp
#define stela_generate_stat_hpp

#include "ast.hpp"
#include "gen context.hpp"
#include "generate expr.hpp"

namespace llvm {

class Function;
class Value;

}

namespace stela {

void generateStat(
  gen::Ctx,
  gen::Func,
  ast::Receiver &,
  ast::FuncParams &,
  ast::Block &
);

}

#endif
