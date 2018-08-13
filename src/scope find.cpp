//
//  scope find.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope find.hpp"

#include <cassert>

using namespace stela;

sym::Symbol *stela::find(sym::Scope *scope, const sym::Name name) {
  auto *unordered = dynamic_cast<sym::UnorderedScope *>(scope);
  assert(unordered);
  const auto iter = unordered->table.find(name);
  if (iter == unordered->table.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

std::vector<sym::Symbol *> stela::findMany(sym::Scope *scope, const sym::Name name) {
  auto *unordered = dynamic_cast<sym::UnorderedScope *>(scope);
  assert(unordered);
  std::vector<sym::Symbol *> symbols;
  const auto [begin, end] = unordered->table.equal_range(name);
  for (auto i = begin; i != end; ++i) {
    symbols.push_back(i->second.get());
  }
  return symbols;
}
