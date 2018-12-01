//
//  generate zero expr.cpp
//  STELA
//
//  Created by Indi Kernick on 1/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate zero expr.hpp"

#include "generate type.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(gen::Ctx ctx)
    : ctx{ctx} {}

  void visit(ast::BtnType &type) override {
    str = generateType(ctx, &type);
    str += "{}";
  }
  void visit(ast::ArrayType &type) override {
    str = "make_null_array<";
    str += generateType(ctx, type.elem.get());
    str += ">()";
  }
  void visit(ast::FuncType &type) override {
    str += "make_func_closure(&";
    str += generateNullFunc(ctx, type);
    str += ")";
  }
  void visit(ast::NamedType &type) override {
    type.definition->type->accept(*this);
  }
  void visit(ast::StructType &type) override {
    gen::String strut = generateType(ctx, &type);
    strut += "{";
    if (type.fields.empty()) {
      strut += "}";
      return;
    }
    type.fields[0].type->accept(*this);
    strut += str;
    for (auto f = type.fields.cbegin() + 1; f != type.fields.cend(); ++f) {
      strut += ", ";
      f->type->accept(*this);
      strut += str;
    }
    strut += "}";
    str = std::move(strut);
  }
  
  gen::String str;

private:
  gen::Ctx ctx;
};

}

gen::String stela::generateZeroExpr(gen::Ctx ctx, ast::Type *type) {
  Visitor visitor{ctx};
  type->accept(visitor);
  return std::move(visitor.str);
}
