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
    gen::String name;
    name += "f_nul_";
    const gen::String ret = generateTypeOrVoid(ctx, type.ret.get());
    name += ret.size();
    name += "_";
    name += ret;
    for (const ast::ParamType &param : type.params) {
      name += "_";
      gen::String paramType = generateType(ctx, param.type.get());
      if (param.ref == ast::ParamRef::inout) {
        paramType += "_ref";
      }
      name += paramType.size();
      name += "_";
      name += paramType;
    }
    if (ctx.inst.funcNotInst(name)) {
      ctx.func += "[[noreturn]] ";
      ctx.func += ret;
      ctx.func += ' ';
      ctx.func += name;
      ctx.func += "(void *";
      for (const ast::ParamType &param : type.params) {
        const gen::String paramType = generateType(ctx, param.type.get());
        ctx.func += ", ";
        ctx.func += paramType;
        if (param.ref == ast::ParamRef::inout) {
          ctx.func += " &";
        }
      }
      ctx.func += ") noexcept {\n";
      ctx.func += "panic(\"Calling null function pointer\");\n";
      ctx.func += "}\n";
    }
    str += "make_func_closure(&";
    str += name;
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
