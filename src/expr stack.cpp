//
//  expr stack.cpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "expr stack.hpp"

#include <cassert>

using namespace stela;

void ExprStack::pushCall() {
  exprs.push_back(Expr{ExprKind::call});
}

void ExprStack::pushExpr(const sym::ExprType &type) {
  exprs.push_back(Expr{ExprKind::expr});
  exprType = type;
}

void ExprStack::pushMember(const sym::Name &name) {
  exprs.push_back(Expr{ExprKind::member, name});
}

void ExprStack::pushMemberExpr(const ast::TypePtr &type) {
  pushExpr(fieldType(exprType, type));
}

void ExprStack::pushFunc(const sym::Name &name) {
  exprs.push_back(Expr{ExprKind::func, name});
}

void ExprStack::pushBtnFunc(const sym::Name &name) {
  exprs.push_back(Expr{ExprKind::btn_func, name});
}

ExprKind ExprStack::top() const {
  assert(!exprs.empty());
  return exprs.back().kind;
}

bool ExprStack::memVarExpr(const ExprKind kind) const {
  const auto top = exprs.rbegin();
  return exprs.size() >= 2
    && top[0].kind == kind
    && top[1].kind == ExprKind::member
    && (exprs.size() == 2 || top[2].kind != ExprKind::call)
  ;
}

bool ExprStack::memFunExpr(const ExprKind kind) const {
  const auto top = exprs.rbegin();
  return exprs.size() >= 3
    && top[0].kind == kind
    && top[1].kind == ExprKind::member
    && top[2].kind == ExprKind::call
  ;
}

bool ExprStack::call(const ExprKind kind) const {
  const auto top = exprs.rbegin();
  return exprs.size() >= 2
    && top[0].kind == kind
    && top[1].kind == ExprKind::call
  ;
}

void ExprStack::popCall() {
  pop(ExprKind::call);
}

sym::Name ExprStack::popMember() {
  return popName(ExprKind::member);
}

sym::Name ExprStack::popFunc() {
  return popName(ExprKind::func);
}

sym::Name ExprStack::popBtnFunc() {
  return popName(ExprKind::btn_func);
}

sym::ExprType ExprStack::popExpr() {
  pop(ExprKind::expr);
  return exprType;
}

ExprStack::Expr::Expr(const ExprKind kind)
  : kind{kind}, name{} {
  assert(
    kind == ExprKind::call ||
    kind == ExprKind::expr ||
    kind == ExprKind::subexpr
  );
}

ExprStack::Expr::Expr(const ExprKind kind, const sym::Name &name)
  : kind{kind}, name{name} {
  assert(
    kind == ExprKind::member ||
    kind == ExprKind::func   ||
    kind == ExprKind::btn_func
  );
}

void ExprStack::setExpr(const sym::ExprType type) {
  assert(top() != ExprKind::expr);
  pushExpr(type);
}

void ExprStack::enterSubExpr() {
  exprs.push_back(Expr{ExprKind::subexpr});
}

sym::ExprType ExprStack::leaveSubExpr() {
  pop(ExprKind::expr);
  pop(ExprKind::subexpr);
  return std::exchange(exprType, sym::null_type);
}

ast::TypePtr ExprStack::topType() const {
  assert(top() == ExprKind::expr);
  return exprType.type;
}

void ExprStack::pop([[maybe_unused]] const ExprKind kind) {
  assert(top() == kind);
  exprs.pop_back();
}

sym::Name ExprStack::popName([[maybe_unused]] const ExprKind kind) {
  assert(top() == kind);
  sym::Name name = std::move(exprs.back().name);
  exprs.pop_back();
  return name;
}
