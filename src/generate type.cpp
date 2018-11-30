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
  Visitor(std::string &bin, Log &log)
    : bin{bin}, log{log} {}
  
  void visit(ast::BtnType &type) override {
    // @TODO X macros?
    switch (type.value) {
      case ast::BtnTypeEnum::Void:
        name = "t_void"; return;
      case ast::BtnTypeEnum::Bool:
        name = "t_void"; return;
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
    std::string elem = std::move(name);
    name = "ArrayPtr<";
    name.append(elem);
    name.push_back('>');
  }
  void visit(ast::FuncType &) override {
    UNREACHABLE();
  }
  void visit(ast::NamedType &type) override {
    name = "t_";
    name.append(type.name);
  }
  void visit(ast::StructType &type) override {
    std::vector<std::string> fields;
    fields.reserve(type.fields.size());
    for (const ast::Field &field : type.fields) {
      field.type->accept(*this);
      fields.push_back(name);
    }
    name = "anonymous_struct_";
    name.append(std::to_string(bin.size()));
    bin.append("struct ");
    bin.append(name);
    bin.append(" {\n");
    for (size_t f = 0; f != fields.size(); ++f) {
      bin.append(fields[f]);
      bin.append(" m_");
      bin.append(type.fields[f].name);
      bin.append(";\n");
    }
    bin.append("};\n");
  }
  
  std::string name;
  
private:
  std::string &bin;
  Log &log;
};

}

std::string stela::generateType(std::string &bin, Log &log, const ast::TypePtr &type) {
  assert(type);
  Visitor visitor{bin, log};
  type->accept(visitor);
  return visitor.name;
}
