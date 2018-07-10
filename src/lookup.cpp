//
//  lookup.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lookup.hpp"

#include "compare ast.hpp"

using namespace stela;

sym::Func *stela::lookup(
  const sym::Table &table,
  const sym::Name name,
  const sym::FuncParams &params
) {
  const auto [begin, end] = table.equal_range(name);
  if (begin == end) {
    return nullptr;
  }
  for (auto f = begin; f != end; ++f) {
    sym::Func *const func = dynamic_cast<sym::Func *>(f->second.get());
    if (!func) {
      continue;
    }
    if (func->params.size() != params.size()) {
      continue;
    }
    bool eq = true;
    for (size_t i = 0; i != params.size(); ++i) {
      if (!equal(params[i], func->params[i])) {
        eq = false;
        break;
      }
    }
    if (eq) {
      return func;
    }
  }
  return nullptr;
}

