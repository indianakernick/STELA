//
//  symbol desc.cpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "symbol desc.hpp"

#include <cassert>
#include "unreachable.hpp"
#include "plain format.hpp"

std::string_view stela::symbolDesc(const sym::Symbol *symbol) {
  // @TODO visitor maybe?
  assert(symbol);
  if (auto *type = dynamic_cast<const sym::TypeAlias *>(symbol)) {
    // @TODO say "builtin type" for builtin types
    return "type";
  }
  if (auto *object = dynamic_cast<const sym::Object *>(symbol)) {
    return "variable";
  }
  if (auto *func = dynamic_cast<const sym::Func *>(symbol)) {
    return "function";
  }
  /* LCOV_EXCL_START */
  if (auto *lambda = dynamic_cast<const sym::Lambda *>(symbol)) {
    return "lambda";
  }
  /* LCOV_EXCL_END */
  if (auto *btnFunc = dynamic_cast<const sym::BtnFunc *>(symbol)) {
    // If we say "builtin function" here, we'll still be saying "type" when
    // talking about bool so we'll have to find a way to say "builtin type" when
    // dealing with sint
    return "function";
  }
  UNREACHABLE();
}

std::string stela::typeDesc(const ast::TypePtr &type) {
  return plainFormatInline(format(type.get()));
}
