//
//  modules.cpp
//  STELA
//
//  Created by Indi Kernick on 21/10/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "modules.hpp"

#include "Log/log output.hpp"
#include "Utils/algorithms.hpp"

using namespace stela;

namespace {

class Visitor {
public:
  Visitor(const ASTs &asts, const ast::Names &compiled, LogSink &sink)
    : order{},
      stack{},
      visited(asts.size(), false),
      asts{asts},
      compiled{compiled},
      log{sink, LogCat::semantic} {
    order.reserve(asts.size());
    stack.reserve(asts.size());
  }

  void checkCycle(const size_t index, const ast::Name &name) {
    if (contains(stack, index)) {
      log.error() << "Cyclic dependencies detected in module \"" << name << "\"" << fatal;
    }
  }

  void visit(const ast::Name &name) {
    for (size_t i = 0; i != asts.size(); ++i) {
      if (asts[i].name == name) {
        checkCycle(i, name);
        return visit(i);
      }
    }
    if (!contains(compiled, name)) {
      log.error() << "Module \"" << name << "\" not found" << fatal;
    }
  }

  void visit(const size_t index) {
    if (visited[index]) {
      return;
    }
    visited[index] = true;
    stack.push_back(index);
    for (const ast::Name &dep : asts[index].imports) {
      visit(dep);
    }
    stack.pop_back();
    order.push_back(index);
  }
  
  void visit() {
    for (size_t i = 0; i != asts.size(); ++i) {
      visit(i);
    }
  }
  
  ModuleOrder order;

private:
  std::vector<size_t> stack;
  std::vector<bool> visited;
  const ASTs &asts;
  const ast::Names &compiled;
  Log log;
};

void checkDuplicateModules(const ASTs &asts, const ast::Names &compiled, LogSink &sink) {
  ast::Names names;
  names.reserve(asts.size() + compiled.size());
  for (const AST &ast : asts) {
    names.push_back(ast.name);
  }
  names.insert(names.end(), compiled.cbegin(), compiled.cend());
  sort(names);
  const auto dup = adjacent_find(names);
  if (dup != names.cend()) {
    Log log{sink, LogCat::semantic};
    log.error() << "Duplicate module \"" << *dup << "\"" << fatal;
  }
}

}

ModuleOrder stela::findModuleOrder(const ASTs &asts, LogSink &sink) {
  return findModuleOrder(asts, {}, sink);
}

ModuleOrder stela::findModuleOrder(const ASTs &asts, const ast::Names &compiled, LogSink &sink) {
  checkDuplicateModules(asts, compiled, sink);
  Visitor visitor{asts, compiled, sink};
  visitor.visit();
  return visitor.order;
}
