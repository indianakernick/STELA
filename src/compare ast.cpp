//
//  compare ast.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "compare ast.hpp"

#include <string>
#include <cassert>

using namespace stela;

namespace {

class TypeEqualVisitor final : public ast::Visitor {
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
          newName += ",";
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

bool stela::equal(const ast::TypePtr &a, const ast::TypePtr &b) {
  TypeEqualVisitor visA;
  TypeEqualVisitor visB;
  a->accept(visA);
  b->accept(visB);
  return visA.name == visB.name;
}
