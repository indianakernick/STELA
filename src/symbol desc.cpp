//
//  symbol desc.cpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "symbol desc.hpp"

#include <cassert>

std::string_view stela::symbolDesc(const sym::Symbol *symbol) {
  assert(symbol);
  if (auto *type = dynamic_cast<const sym::TypeAlias *>(symbol)) {
    return "type";
  }
  if (auto *object = dynamic_cast<const sym::Object *>(symbol)) {
    return "variable";
  }
  if (auto *func = dynamic_cast<const sym::Func *>(symbol)) {
    return "function";
  }
  if (auto *lambda = dynamic_cast<const sym::Lambda *>(symbol)) {
    return "lambda";
  }
  /* LCOV_EXCL_START */
  assert(false);
  return "";
  /* LCOV_EXCL_END */
}
