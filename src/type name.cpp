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
    name = std::string(namedType.name);
  }
  
  std::string name;
};

}

std::string stela::typeName(const ast::TypePtr &type) {
  TypeNameVisitor visitor;
  type->accept(visitor);
  return visitor.name;
}

sym::FuncParams stela::funcParams(const ast::FuncParams &params) {
  sym::FuncParams symParams;
  for (const ast::FuncParam &param : params) {
    symParams.push_back(typeName(param.type));
  }
  return symParams;
}
