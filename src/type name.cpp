//
//  type name.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "type name.hpp"

#include <cassert>
#include "lookup.hpp"

using namespace stela;

namespace {

class TypeNameVisitor final : public ast::Visitor {
public:
  TypeNameVisitor(const sym::Scope &scope, Log &log)
    : scope{scope}, log{log} {}

  void visit(ast::ArrayType &array) override {
    std::string newName;
    newName += '[';
    assert(array.elem);
    array.elem->accept(*this);
    newName += std::move(name);
    newName += ']';
    name = std::move(newName);
  }
  
  void visit(ast::MapType &map) override {
    std::string newName;
    newName += "[{";
    assert(map.key);
    map.key->accept(*this);
    newName += std::move(name);
    newName += ':';
    assert(map.val);
    map.val->accept(*this);
    newName += std::move(name);
    newName += "}]";
    name = std::move(newName);
  }
  
  void visit(ast::FuncType &func) override {
    std::string newName;
    newName += '(';
    if (!func.params.empty()) {
      for (auto p = func.params.begin(); p != func.params.end(); ++p) {
        if (p->ref == ast::ParamRef::inout) {
          newName += "inout ";
        }
        assert(p->type);
        p->type->accept(*this);
        newName += std::move(name);
        if (p != func.params.end() - 1) {
          newName += ',';
        }
      }
    }
    newName += "->";
    assert(func.ret);
    func.ret->accept(*this);
    newName += std::move(name);
    name = std::move(newName);
  }
  
  void visit(ast::NamedType &namedType) override {
    sym::Symbol *sym = lookupUse(scope, namedType.name, namedType.loc, log);
    assert(sym);
    if (auto *alias = dynamic_cast<sym::TypeAlias *>(sym); alias) {
      name = alias->type;
    } else {
      name = std::string(namedType.name);
    }
  }
  
  std::string name;

private:
  const sym::Scope &scope;
  Log &log;
};

}

std::string stela::typeName(const sym::Scope &scope, const ast::TypePtr &type, Log &log) {
  TypeNameVisitor visitor{scope, log};
  type->accept(visitor);
  return visitor.name;
}

sym::FuncParams stela::funcParams(const sym::Scope &scope, const ast::FuncParams &params, Log &log) {
  sym::FuncParams symParams;
  for (const ast::FuncParam &param : params) {
    symParams.push_back(typeName(scope, param.type, log));
  }
  return symParams;
}
