//
//  compare types.cpp
//  STELA
//
//  Created by Indi Kernick on 5/9/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "compare types.hpp"

#include <typeinfo>

using namespace stela;

namespace {

template <typename Left>
class RightVisitor final : public ast::Visitor {
public:
  RightVisitor(const NameLookup &lkp, Left &left)
    : lkp{lkp}, left{left} {}

  void visit(ast::BuiltinType &right) override {
    eq = compare(left, right);
  }
  void visit(ast::ArrayType &right) override {
    eq = compare(left, right);
  }
  void visit(ast::FuncType &right) override {
    eq = compare(left, right);
  }
  void visit(ast::NamedType &right) override {
    ast::TypeAlias *def = lkp.lookupType(right);
    if (def->strong) {
      eq = compare(left, right); // a strong type alias is its own type
    } else {
      def->type->accept(*this);  // a weak type alias is an existing type
    }
  }
  void visit(ast::NamespacedType &right) override {
    
  }
  void visit(ast::StructType &right) override {
    eq = compare(left, right);
  }

  bool eq = false;

private:
  const NameLookup &lkp;
  Left &left;

  bool compare(ast::BuiltinType &left, ast::BuiltinType &right) {
    return left.value == right.value;
  }
  bool compare(ast::ArrayType &left, ast::ArrayType &right) {
    return compareTypes(lkp, left.elem.get(), right.elem.get());
  }
  bool compare(ast::FuncType &left, ast::FuncType &right) {
    if (left.params.size() != right.params.size()) {
      return false;
    }
    for (size_t i = 0; i != left.params.size(); ++i) {
      if (left.params[i].ref != right.params[i].ref) {
        return false;
      }
      if (!compareTypes(lkp, left.params[i].type.get(), right.params[i].type.get())) {
        return false;
      }
    }
    return compareTypes(lkp, left.ret.get(), right.ret.get());
  }
  bool compare(ast::NamedType &left, ast::NamedType &right) {
    return left.name == right.name;
  }
  bool compare(ast::NamespacedType &left, ast::NamespacedType &right) {
    return false;
  }
  bool compare(ast::StructType &left, ast::StructType &right) {
    if (left.fields.size() != right.fields.size()) {
      return false;
    }
    for (size_t i = 0; i != left.fields.size(); ++i) {
      if (left.fields[i].name != right.fields[i].name) {
        return false;
      }
      if (!compareTypes(lkp, left.fields[i].type.get(), right.fields[i].type.get())) {
        return false;
      }
    }
    return true;
  }
  template <typename Right>
  bool compare(Left &, Right &) {
    return false;
  }
};

class LeftVisitor final : public ast::Visitor {
public:
  LeftVisitor(const NameLookup &lkp, ast::Type *right)
    : lkp{lkp}, right{right} {}

  void visit(ast::BuiltinType &left) override {
    visitImpl(left);
  }
  void visit(ast::ArrayType &left) override {
    visitImpl(left);
  }
  void visit(ast::FuncType &left) override {
    visitImpl(left);
  }
  void visit(ast::NamedType &left) override {
    ast::TypeAlias *def = lkp.lookupType(left);
    if (def->strong) {
      visitImpl(left);          // a strong type alias is its own type
    } else {
      def->type->accept(*this); // a weak type alias is an existing type
    }
  }
  void visit(ast::NamespacedType &left) override {
    
  }
  void visit(ast::StructType &left) override {
    visitImpl(left);
  }
  
  bool eq = false;

private:
  template <typename Type>
  void visitImpl(Type &left) {
    RightVisitor rVis{lkp, left};
    right->accept(rVis);
    eq = rVis.eq;
  }
  
  const NameLookup &lkp;
  ast::Type *right;
};

}

bool stela::compareTypes(const NameLookup &lkp, ast::Type *a, ast::Type *b) {
  if (a == b) {
    return true;
  }
  if (a == nullptr || b == nullptr) {
    return false;
  }
  LeftVisitor left{lkp, b};
  a->accept(left);
  return left.eq;
}
