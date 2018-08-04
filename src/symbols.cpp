//
//  symbols.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "symbols.hpp"

#define ACCEPT(TYPE)                                                            \
  void stela::sym::TYPE::accept(ScopeVisitor &visitor) {                        \
    visitor.visit(*this);                                                       \
  }

/* LCOV_EXCL_START */

ACCEPT(NSScope)
ACCEPT(BlockScope)
ACCEPT(FuncScope)
ACCEPT(StructScope)
ACCEPT(EnumScope)

/* LCOV_EXCL_END */
