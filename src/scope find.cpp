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

namespace {

class FindVisitor final : public sym::ScopeVisitor {
public:
  explicit FindVisitor(const sym::Name &name)
    : name{name} {}

  void visit(sym::NSScope &scope) override {
    find(scope.table);
  }
  void visit(sym::BlockScope &scope) override {
    find(scope.table);
  }
  void visit(sym::FuncScope &scope) override {
    find(scope.table);
  }
  void visit(sym::StructScope &scope) override {
    for (const sym::StructTableRow &row : scope.table) {
      if (row.name == name && row.scope == sym::MemScope::static_) {
        symbol = row.val.get();
        return;
      }
    }
    symbol = nullptr;
  }
  void visit(sym::EnumScope &scope) override {
    for (const sym::EnumTableRow &row : scope.table) {
      if (row.key == name) {
        symbol = row.val.get();
        return;
      }
    }
    symbol = nullptr;
  }
  
  sym::Symbol *symbol;

private:
  const sym::Name &name;
  
  void find(const sym::UnorderedTable &table) {
    const auto iter = table.find(name);
    if (iter == table.end()) {
      symbol = nullptr;
    } else {
      symbol = iter->second.get();
    }
  }
};

class FindManyVisitor final : public sym::ScopeVisitor {
public:
  explicit FindManyVisitor(const sym::Name &name)
    : name{name} {}

  void visit(sym::NSScope &scope) override {
    find(scope.table);
  }
  void visit(sym::BlockScope &scope) override {
    find(scope.table);
  }
  void visit(sym::FuncScope &scope) override {
    find(scope.table);
  }
  void visit(sym::StructScope &) override {
    assert(false);
  }
  void visit(sym::EnumScope &scope) override {
    for (const sym::EnumTableRow &row : scope.table) {
      if (row.key == name) {
        symbols.push_back(row.val.get());
      }
    }
  }

  std::vector<sym::Symbol *> symbols;

private:
  const sym::Name &name;
  
  void find(const sym::UnorderedTable &table) {
    const auto [begin, end] = table.equal_range(name);
    for (auto i = begin; i != end; ++i) {
      symbols.push_back(i->second.get());
    }
  }
};

}

sym::Symbol *stela::find(sym::Scope *scope, const sym::Name name) {
  FindVisitor finder{name};
  scope->accept(finder);
  return finder.symbol;
}

std::vector<sym::Symbol *> stela::findMany(sym::Scope *scope, const sym::Name name) {
  FindManyVisitor finder{name};
  scope->accept(finder);
  return finder.symbols;
}
