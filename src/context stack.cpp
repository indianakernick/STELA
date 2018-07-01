//
//  context stack.cpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "context stack.hpp"

#include <cassert>
#include <iostream>

stela::Context::~Context() {
  assert(stack.stack.size() == index + 1);
  stack.stack.pop_back();
}

void stela::Context::desc(const std::string_view desc) {
  assert(index < stack.stack.size());
  stack.stack[index].desc = desc;
}

void stela::Context::ident(const std::string_view ident) {
  assert(index < stack.stack.size());
  stack.stack[index].ident = ident;
}

stela::Context::Context(ContextStack &stack, const size_t index)
  : stack{stack}, index{index} {}

stela::Context stela::ContextStack::context(const std::string_view desc) {
  const size_t index = stack.size();
  stack.push_back(ContextData{desc, {}});
  return Context{*this, index};
}

std::ostream &stela::operator<<(std::ostream &stream, const ContextStack &stack) {
  if (!stack.stack.empty()) {
    stream << ' ';
  }
  for (auto c = stack.stack.crbegin(); c != stack.stack.crend(); ++c) {
    stream << c->desc;
    if (!c->ident.empty()) {
      stream << " \"" << c->ident << '"';
    }
    if (c != stack.stack.crend() - 1) {
      stream << ", ";
    }
  }
  return stream;
}
