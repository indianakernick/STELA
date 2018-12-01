//
//  generate type.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate type.hpp"

#include "unreachable.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(gen::Ctx ctx)
    : ctx{ctx} {}
  
  void visit(ast::BtnType &type) override {
    // @TODO X macros?
    switch (type.value) {
      case ast::BtnTypeEnum::Void:
        name = "t_void"; return;
      case ast::BtnTypeEnum::Bool:
        name = "t_bool"; return;
      case ast::BtnTypeEnum::Byte:
        name = "t_byte"; return;
      case ast::BtnTypeEnum::Char:
        name = "t_char"; return;
      case ast::BtnTypeEnum::Real:
        name = "t_real"; return;
      case ast::BtnTypeEnum::Sint:
        name = "t_sint"; return;
      case ast::BtnTypeEnum::Uint:
        name = "t_uint"; return;
    }
    UNREACHABLE();
  }
  void visit(ast::ArrayType &type) override {
    type.elem->accept(*this);
    gen::String elem = std::move(name);
    name = "t_arr_";
    name += elem.size();
    name += '_';
    name += elem;
    if (ctx.inst.arrayNotInst(name)) {
      ctx.type += "using ";
      ctx.type += name;
      ctx.type += " = ArrayPtr<";
      ctx.type += elem;
      ctx.type += ">;\n";
    }
  }
  void visit(ast::FuncType &type) override {
    name = "Closure<";
    name += generateFuncSig(ctx, type);
    name += ">";
  }
  void visit(ast::NamedType &type) override {
    type.definition->type->accept(*this);
  }
  void visit(ast::StructType &type) override {
    std::vector<gen::String> fields;
    fields.reserve(type.fields.size());
    for (const ast::Field &field : type.fields) {
      field.type->accept(*this);
      fields.push_back(std::move(name));
    }
    name = "t_srt";
    for (size_t f = 0; f != fields.size(); ++f) {
      name += '_';
      name += fields[f].size();
      name += '_';
      name += fields[f];
      name += '_';
      name += type.fields[f].name.size();
      name += '_';
      name += type.fields[f].name;
    }
    if (ctx.inst.structNotInst(name)) {
      ctx.type += "struct ";
      ctx.type += name;
      ctx.type += " {\n";
      for (size_t f = 0; f != fields.size(); ++f) {
        ctx.type += fields[f];
        ctx.type += " m_";
        ctx.type += f;
        ctx.type += ";\n";
      }
      ctx.type += "};\n";
    }
  }
  
  gen::String name;
  
private:
  gen::Ctx ctx;
};

}

gen::String stela::generateType(gen::Ctx ctx, ast::Type *type) {
  assert(type);
  Visitor visitor{ctx};
  type->accept(visitor);
  return std::move(visitor.name);
}

gen::String stela::generateTypeOrVoid(gen::Ctx ctx, ast::Type *type) {
  if (type) {
    return generateType(ctx, type);
  } else {
    return gen::String{"t_void"};
  }
}

gen::String stela::generateFuncSig(gen::Ctx ctx, const ast::FuncType &type) {
  gen::String str{10 + 10 * type.params.size()};
  if (type.ret) {
    str += generateType(ctx, type.ret.get());
  } else {
    str += "void";
  }
  str += "(void *";
  for (const ast::ParamType &param : type.params) {
    str += ", ";
    str += generateType(ctx, param.type.get());
    if (param.ref == ast::ParamRef::inout) {
      str += " &";
    }
  }
  str += ") noexcept";
  return str;
}
