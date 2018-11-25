//
//  compare types.cpp
//  STELA
//
//  Created by Indi Kernick on 5/9/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "compare types.hpp"

#include "scope lookup.hpp"
#include <Simpleton/Utils/algorithm.hpp>

using namespace stela;

namespace {

template <typename Left>
class RightVisitor final : public ast::Visitor {
public:
  RightVisitor(sym::Ctx ctx, Left &left)
    : ctx{ctx}, left{left} {}

  void visit(ast::BtnType &right) override {
    eq = compare(ctx, left, right);
  }
  void visit(ast::ArrayType &right) override {
    eq = compare(ctx, left, right);
  }
  void visit(ast::FuncType &right) override {
    eq = compare(ctx, left, right);
  }
  void visit(ast::NamedType &right) override {
    ast::TypeAlias *def = lookupTypeName(ctx, right);
    if (def->strong) {
      eq = compare(ctx, left, right); // a strong type alias is its own type
    } else {
      def->type->accept(*this);  // a weak type alias is an existing type
    }
  }
  void visit(ast::StructType &right) override {
    eq = compare(ctx, left, right);
  }

  bool eq = false;

private:
  sym::Ctx ctx;
  Left &left;

  static bool compare(const sym::Ctx &, ast::BtnType &left, ast::BtnType &right) {
    return left.value == right.value;
  }
  static bool compare(const sym::Ctx &ctx, ast::ArrayType &left, ast::ArrayType &right) {
    return compareTypes(ctx, left.elem, right.elem);
  }
  static bool compare(const sym::Ctx &ctx, ast::FuncType &left, ast::FuncType &right) {
    const auto compareParams = [&ctx] (const ast::ParamType &a, const ast::ParamType &b) {
      return a.ref == b.ref && compareTypes(ctx, a.type, b.type);
    };
    if (!compareTypes(ctx, left.ret, right.ret)) {
      return false;
    }
    return Utils::equal_size(left.params, right.params, compareParams);
  }
  static bool compare(const sym::Ctx &, ast::NamedType &left, ast::NamedType &right) {
    return left.name == right.name;
  }
  static bool compare(const sym::Ctx &ctx, ast::StructType &left, ast::StructType &right) {
    const auto compareFields = [&ctx] (const ast::Field &a, const ast::Field &b) {
      return a.name == b.name && compareTypes(ctx, a.type, b.type);
    };
    return Utils::equal_size(left.fields, right.fields, compareFields);
  }
  template <typename Right>
  static bool compare(const sym::Ctx &, Left &, Right &) {
    return false;
  }
};

class LeftVisitor final : public ast::Visitor {
public:
  LeftVisitor(sym::Ctx ctx, ast::Type *right)
    : ctx{ctx}, right{right} {}

  void visit(ast::BtnType &left) override {
    visitImpl(left);
  }
  void visit(ast::ArrayType &left) override {
    visitImpl(left);
  }
  void visit(ast::FuncType &left) override {
    visitImpl(left);
  }
  void visit(ast::NamedType &left) override {
    ast::TypeAlias *def = lookupTypeName(ctx, left);
    if (def->strong) {
      visitImpl(left);          // a strong type alias is its own type
    } else {
      def->type->accept(*this); // a weak type alias is an existing type
    }
  }
  void visit(ast::StructType &left) override {
    visitImpl(left);
  }
  
  bool eq = false;

private:
  template <typename Type>
  void visitImpl(Type &left) {
    RightVisitor rVis{ctx, left};
    right->accept(rVis);
    eq = rVis.eq;
  }
  
  sym::Ctx ctx;
  ast::Type *right;
};

}

bool stela::compareTypes(sym::Ctx ctx, const ast::TypePtr &a, const ast::TypePtr &b) {
  if (a == b) {
    return true;
  }
  if (a == nullptr || b == nullptr) {
    return false;
  }
  LeftVisitor left{ctx, b.get()};
  a->accept(left);
  return left.eq;
}

namespace {

class ValidVisitor final : public ast::Visitor {
public:
  explicit ValidVisitor(sym::Ctx ctx)
    : ctx{ctx} {}

  void visit(ast::BtnType &) override {}
  void visit(ast::ArrayType &type) override {
    type.elem->accept(*this);
  }
  void visit(ast::FuncType &type) override {
    if (type.ret) {
      type.ret->accept(*this);
    }
    for (const ast::ParamType &param : type.params) {
      param.type->accept(*this);
    }
  }
  void visit(ast::NamedType &type) override {
    ast::TypeAlias *const alias = lookupTypeName(ctx, type);
    assert(alias);
    alias->type->accept(*this);
  }
  void visit(ast::StructType &type) override {
    std::vector<std::pair<std::string_view, Loc>> names;
    names.reserve(type.fields.size());
    for (const ast::Field &field : type.fields) {
      field.type->accept(*this);
      names.push_back({field.name, field.loc});
    }
    Utils::sort(names, [] (auto a, auto b) {
      return a.first < b.first;
    });
    const auto dup = Utils::adjacent_find(names, [] (auto a, auto b) {
      return a.first == b.first;
    });
    if (dup != names.cend()) {
      ctx.log.error(dup->second) << "Duplicate field \"" << dup->first << "\" in struct" << fatal;
    }
  }

private:
  sym::Ctx ctx;
};

}

void stela::validateType(sym::Ctx ctx, const ast::TypePtr &type) {
  if (type) {
    ValidVisitor visitor{ctx};
    type->accept(visitor);
  }
}
