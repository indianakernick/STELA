//
//  clone type.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "clone type.hpp"

using namespace stela;

namespace {

class CloneVisitor final : public ast::Visitor {
public:
  void visit(ast::ArrayType &array) {
    auto newType = std::make_unique<ast::ArrayType>();
    newType->loc = array.loc;
    array.elem->accept(*this);
    newType->elem = std::move(type);
    type = std::move(newType);
  }
  void visit(ast::MapType &map) {
    auto newType = std::make_unique<ast::MapType>();
    newType->loc = map.loc;
    map.key->accept(*this);
    newType->key = std::move(type);
    map.val->accept(*this);
    newType->val = std::move(type);
    type = std::move(newType);
  }
  void visit(ast::FuncType &func) {
    auto newType = std::make_unique<ast::FuncType>();
    newType->loc = func.loc;
    if (func.ret) {
      func.ret->accept(*this);
      newType->ret = std::move(type);
    }
    for (const ast::ParamType &param : func.params) {
      ast::ParamType &newParam = newType->params.emplace_back();
      newParam.ref = param.ref;
      param.type->accept(*this);
      newParam.type = std::move(type);
    }
    type = std::move(newType);
  }
  void visit(ast::NamedType &name) {
    auto newType = std::make_unique<ast::NamedType>();
    newType->loc = name.loc;
    newType->name = name.name;
    type = std::move(newType);
  }
  
  ast::TypePtr type;
};

}

ast::TypePtr stela::clone(const ast::TypePtr &type) {
  CloneVisitor visitor;
  type->accept(visitor);
  return std::move(visitor.type);
}
