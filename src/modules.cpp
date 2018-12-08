//
//  modules.cpp
//  STELA
//
//  Created by Indi Kernick on 21/10/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "modules.hpp"

#include "log output.hpp"
#include "algorithms.hpp"

using namespace stela;

namespace {

class Visitor {
public:
  Visitor(const ASTs &asts, const ast::Names &compiled, LogBuf &buf)
    : order{},
      stack{},
      visited(asts.size(), false),
      asts{asts},
      compiled{compiled},
      log{buf, LogCat::semantic} {
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

void checkDuplicateModules(const ASTs &asts, const ast::Names &compiled, LogBuf &buf) {
  ast::Names names;
  names.reserve(asts.size() + compiled.size());
  for (const AST &ast : asts) {
    names.push_back(ast.name);
  }
  names.insert(names.end(), compiled.cbegin(), compiled.cend());
  sort(names);
  const auto dup = adjacent_find(names);
  if (dup != names.cend()) {
    Log log{buf, LogCat::semantic};
    log.error() << "Duplicate module \"" << *dup << "\"" << fatal;
  }
}

}

ModuleOrder stela::findModuleOrder(const ASTs &asts, LogBuf &buf) {
  return findModuleOrder(asts, {}, buf);
}

ModuleOrder stela::findModuleOrder(const ASTs &asts, const ast::Names &compiled, LogBuf &buf) {
  checkDuplicateModules(asts, compiled, buf);
  Visitor visitor{asts, compiled, buf};
  visitor.visit();
  return visitor.order;
}
