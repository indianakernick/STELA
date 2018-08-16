//
//  infer type.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "infer type.hpp"

#include "scope lookup.hpp"
#include "operator name.hpp"
#include "number literal.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(ScopeMan &man, Log &log, const BuiltinTypes &types)
    : lkp{man.cur(), log}, tlk{man, log}, man{man}, log{log}, bnt{types} {}

  void visit(ast::Assignment &as) override {
    lkp.enterSubExpr();
    as.left->accept(*this);
    const sym::ExprType left = lkp.leaveSubExpr();
    lkp.enterSubExpr();
    as.right->accept(*this);
    const sym::ExprType right = lkp.leaveSubExpr();
    as.definition = lkp.lookupFunc(sym::Name(opName(as.oper)), {left, right}, as.loc);
  }
  void visit(ast::BinaryExpr &bin) override {
    lkp.enterSubExpr();
    bin.left->accept(*this);
    const sym::ExprType left = lkp.leaveSubExpr();
    lkp.enterSubExpr();
    bin.right->accept(*this);
    const sym::ExprType right = lkp.leaveSubExpr();
    bin.definition = lkp.lookupFunc(sym::Name(opName(bin.oper)), {left, right}, bin.loc);
  }
  void visit(ast::UnaryExpr &un) override {
    lkp.enterSubExpr();
    un.expr->accept(*this);
    const sym::ExprType type = lkp.leaveSubExpr();
    un.definition = lkp.lookupFunc(sym::Name(opName(un.oper)), {type}, un.loc);
  }
  sym::FuncParams argTypes(const ast::FuncArgs &args) {
    sym::FuncParams params;
    for (const ast::ExprPtr &expr : args) {
      params.push_back(getExprType(man, log, expr.get(), bnt));
    }
    return params;
  }
  void visit(ast::FuncCall &call) override {
    lkp.call();
    call.func->accept(*this);
    call.definition = lkp.lookupFunc(argTypes(call.args), call.loc);
  }
  void visit(ast::MemberIdent &mem) override {
    lkp.member(sym::Name(mem.member));
    mem.object->accept(*this);
    mem.definition = lkp.lookupMember(mem.loc);
  }
  void visit(ast::Identifier &id) override {
    id.definition = lkp.lookupIdent(sym::Name(id.name), id.loc);
  }
  void visit(ast::Self &self) override {
    self.definition = lkp.lookupSelf(self.loc);
  }
  void visit(ast::Ternary &tern) override {
    lkp.enterSubExpr();
    tern.cond->accept(*this);
    const sym::ExprType cond = lkp.leaveSubExpr();
    if (cond.type != bnt.Bool) {
      log.error(tern.loc) << "Condition of ternary conditional must be a Bool" << fatal;
    }
    lkp.enterSubExpr();
    tern.tru->accept(*this);
    const sym::ExprType tru = lkp.leaveSubExpr();
    lkp.enterSubExpr();
    tern.fals->accept(*this);
    const sym::ExprType fals = lkp.leaveSubExpr();
    if (tru.type != fals.type) {
      log.error(tern.loc) << "True and false branch of ternary condition must have same type" << fatal;
    }
    sym::ExprType etype;
    etype.type = tru.type;
    etype.mut = sym::common(tru.mut, fals.mut);
    etype.ref = sym::common(tru.ref, fals.ref);
    lkp.setExpr(etype);
  }
  
  void visit(ast::StringLiteral &) override {
    sym::ExprType etype;
    etype.type = bnt.String;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::CharLiteral &) override {
    sym::ExprType etype;
    etype.type = bnt.Char;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::NumberLiteral &n) override {
    const NumberVariant num = parseNumberLiteral(n.value, log);
    sym::ExprType etype;
    if (num.type == NumberVariant::Float) {
      etype.type = bnt.Double;
    } else if (num.type == NumberVariant::Int) {
      etype.type = bnt.Int64;
    } else if (num.type == NumberVariant::UInt) {
      etype.type = bnt.UInt64;
    }
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::BoolLiteral &) override {
    sym::ExprType etype;
    etype.type = bnt.Bool;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::ArrayLiteral &) override {}
  void visit(ast::MapLiteral &) override {}
  void visit(ast::Lambda &) override {}

  void enter() {
    lkp.enterSubExpr();
  }
  sym::ExprType leave() {
    return lkp.leaveSubExpr();
  }

private:
  ExprLookup lkp;
  TypeLookup tlk;
  ScopeMan &man;
  Log &log;
  const BuiltinTypes &bnt;
};

}

sym::ExprType stela::getExprType(
  ScopeMan &man,
  Log &log,
  ast::Expression *expr,
  const BuiltinTypes &types
) {
  Visitor visitor{man, log, types};
  visitor.enter();
  expr->accept(visitor);
  return visitor.leave();
}
