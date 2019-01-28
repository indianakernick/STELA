//
//  compare types.cpp
//  STELA
//
//  Created by Indi Kernick on 5/9/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "compare types.hpp"

#include "scope lookup.hpp"
#include "Utils/algorithms.hpp"

using namespace stela;

namespace {

template <typename Left>
class RightVisitor final : public ast::Visitor {
public:
  RightVisitor(sym::Ctx ctx, Left &lhs)
    : ctx{ctx}, lhs{lhs} {}

  void visit(ast::BtnType &rhs) override {
    eq = compare(ctx, lhs, rhs);
  }
  void visit(ast::ArrayType &rhs) override {
    eq = compare(ctx, lhs, rhs);
  }
  void visit(ast::FuncType &rhs) override {
    eq = compare(ctx, lhs, rhs);
  }
  void visit(ast::NamedType &rhs) override {
    ast::TypeAlias *def = lookupTypeName(ctx, rhs);
    if (def->strong) {
      eq = compare(ctx, lhs, rhs); // a strong type alias is its own type
    } else {
      def->type->accept(*this);    // a weak type alias is an existing type
    }
  }
  void visit(ast::StructType &rhs) override {
    eq = compare(ctx, lhs, rhs);
  }
  void visit(ast::UserType &rhs) override {
    eq = compare(ctx, lhs, rhs);
  }

  bool eq = false;

private:
  sym::Ctx ctx;
  Left &lhs;

  static bool compare(const sym::Ctx &, ast::BtnType &lhs, ast::BtnType &rhs) {
    return lhs.value == rhs.value;
  }
  static bool compare(const sym::Ctx &ctx, ast::ArrayType &lhs, ast::ArrayType &rhs) {
    return compareTypes(ctx, lhs.elem, rhs.elem);
  }
  static bool compare(const sym::Ctx &ctx, ast::FuncType &lhs, ast::FuncType &rhs) {
    const auto compareParams = [&ctx] (const ast::ParamType &a, const ast::ParamType &b) {
      return a.ref == b.ref && compareTypes(ctx, a.type, b.type);
    };
    if (!compareTypes(ctx, lhs.ret, rhs.ret)) {
      return false;
    }
    return equal_size(lhs.params, rhs.params, compareParams);
  }
  static bool compare(const sym::Ctx &, ast::NamedType &lhs, ast::NamedType &rhs) {
    return lhs.name == rhs.name;
  }
  static bool compare(const sym::Ctx &ctx, ast::StructType &lhs, ast::StructType &rhs) {
    const auto compareFields = [&ctx] (const ast::Field &a, const ast::Field &b) {
      return a.name == b.name && compareTypes(ctx, a.type, b.type);
    };
    return equal_size(lhs.fields, rhs.fields, compareFields);
  }
  static bool compare(const sym::Ctx &ctx, ast::UserType &lhs, ast::UserType &rhs) {
    const auto compareFields = [&ctx] (const ast::UserField &a, const ast::UserField &b) {
      return a.offset == b.offset &&
        a.name == b.name &&
        compareTypes(ctx, a.type, b.type);
    };
    return lhs.size == rhs.size &&
      lhs.align == rhs.align &&
      equal_size(lhs.fields, rhs.fields, compareFields);
  }
  template <typename Right>
  static bool compare(const sym::Ctx &, Left &, Right &) {
    return false;
  }
};

class LeftVisitor final : public ast::Visitor {
public:
  LeftVisitor(sym::Ctx ctx, ast::Type *rhs)
    : ctx{ctx}, rhs{rhs} {}

  void visit(ast::BtnType &lhs) override {
    visitImpl(lhs);
  }
  void visit(ast::ArrayType &lhs) override {
    visitImpl(lhs);
  }
  void visit(ast::FuncType &lhs) override {
    visitImpl(lhs);
  }
  void visit(ast::NamedType &lhs) override {
    ast::TypeAlias *def = lookupTypeName(ctx, lhs);
    if (def->strong) {
      visitImpl(lhs);           // a strong type alias is its own type
    } else {
      def->type->accept(*this); // a weak type alias is an existing type
    }
  }
  void visit(ast::StructType &lhs) override {
    visitImpl(lhs);
  }
  void visit(ast::UserType &lhs) override {
    visitImpl(lhs);
  }
  
  bool eq = false;

private:
  template <typename Type>
  void visitImpl(Type &lhs) {
    RightVisitor rVis{ctx, lhs};
    rhs->accept(rVis);
    eq = rVis.eq;
  }
  
  sym::Ctx ctx;
  ast::Type *rhs;
};

}

bool stela::compareTypes(sym::Ctx ctx, const ast::TypePtr &a, const ast::TypePtr &b) {
  if (a == b) {
    return true;
  }
  if (a == nullptr || b == nullptr) {
    return false;
  }
  LeftVisitor lhs{ctx, b.get()};
  a->accept(lhs);
  return lhs.eq;
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
    } else {
      // @TODO maybe the return type of a FuncType should NOT be optional
      type.ret = ctx.btn.Void;
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
    sort(names, [] (auto a, auto b) {
      return a.first < b.first;
    });
    const auto dup = adjacent_find(names, [] (auto a, auto b) {
      return a.first == b.first;
    });
    if (dup != names.cend()) {
      ctx.log.error(dup->second) << "Duplicate field \"" << dup->first << "\" in struct" << fatal;
    }
  }
  void visit(ast::UserType &) override {
    // @TODO do we need to do anything here?
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
