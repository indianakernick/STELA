//
//  type name.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "type name.hpp"

#include <cassert>

using namespace stela;

namespace {

class TypeNameVisitor final : public ast::Visitor {
public:
  explicit TypeNameVisitor(const sym::Scope &scope)
    : scope{scope} {}

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
    // could be typealias, struct or enum
    // need to check symbol table to find out
    name = std::string(namedType.name);
  }
  
  std::string name;

private:
  const sym::Scope &scope;
};

}

std::string stela::typeName(const sym::Scope &scope, const ast::TypePtr &type) {
  TypeNameVisitor visitor{scope};
  type->accept(visitor);
  return visitor.name;
}

std::string stela::funcName(
  const sym::Scope &scope,
  const std::string_view name,
  const ast::FuncParams &params
) {
  // might need to implement a proper name mangling algorithm that produces
  // valid identifiers
  std::string str = std::string(name);
  for (const ast::FuncParam &param : params) {
    str += ' ';
    str += (typeName(scope, param.type));
  }
  return str;
}

sym::FuncParams stela::funcParams(const sym::Scope &scope, const ast::FuncParams &params) {
  sym::FuncParams symParams;
  for (const ast::FuncParam &param : params) {
    symParams.push_back(typeName(scope, param.type));
  }
  return symParams;
}
