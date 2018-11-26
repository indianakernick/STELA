//
//  symbol desc.cpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "symbol desc.hpp"

#include <cassert>

std::string_view stela::symbolDesc(sym::Symbol *const symbol) {
  assert(symbol);
  if (auto *type = dynamic_cast<sym::TypeAlias *>(symbol)) {
    return "type";
  }
  if (auto *object = dynamic_cast<sym::Object *>(symbol)) {
    return "variable";
  }
  if (auto *func = dynamic_cast<sym::Func *>(symbol)) {
    return "function";
  }
  if (auto *lambda = dynamic_cast<sym::Lambda *>(symbol)) {
    return "lambda";
  }
  /* LCOV_EXCL_START */
  assert(false);
  return "";
  /* LCOV_EXCL_END */
}
