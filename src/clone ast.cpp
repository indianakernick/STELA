//
//  clone ast.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "clone ast.hpp"

#include <cassert>

using namespace stela;

namespace {

class TypeCloneVisitor final : public ast::Visitor {
public:
  void visit(ast::ArrayType &array) override {
    auto newArray = std::make_unique<ast::ArrayType>();
    newArray->loc = array.loc;
    assert(array.elem);
    array.elem->accept(*this);
    newArray->elem = std::move(type);
    type = std::move(newArray);
  }
  
  void visit(ast::MapType &map) override {
    auto newMap = std::make_unique<ast::MapType>();
    newMap->loc = map.loc;
    assert(map.key);
    map.key->accept(*this);
    newMap->key = std::move(type);
    assert(map.val);
    map.val->accept(*this);
    newMap->val = std::move(type);
    type = std::move(newMap);
  }
  
  void visit(ast::FuncType &func) override {
    auto newFunc = std::make_unique<ast::FuncType>();
    newFunc->loc = func.loc;
    assert(func.ret);
    func.ret->accept(*this);
    newFunc->ret = std::move(type);
    for (const ast::ParamType &param : func.params) {
      ast::ParamType &newParam = newFunc->params.emplace_back();
      newParam.ref = param.ref;
      assert(param.type);
      param.type->accept(*this);
      newParam.type = std::move(type);
    }
    type = std::move(newFunc);
  }
  
  void visit(ast::NamedType &name) override {
    auto newName = std::make_unique<ast::NamedType>();
    newName->loc = name.loc;
    newName->name = name.name;
    type = std::move(newName);
  }
  
  ast::TypePtr type;
};

}

ast::TypePtr stela::clone(const ast::TypePtr &type) {
  TypeCloneVisitor visitor;
  type->accept(visitor);
  return std::move(visitor.type);
}
