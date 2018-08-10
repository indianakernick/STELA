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
    const sym::ExprType left = getExprType(man, log, as.left.get(), bnt);
    const sym::ExprType right = getExprType(man, log, as.right.get(), bnt);
    as.definition = lkp.lookupFunc(sym::Name(opName(as.oper)), {left, right}, as.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::BinaryExpr &bin) override {
    const sym::ExprType left = getExprType(man, log, bin.left.get(), bnt);
    const sym::ExprType right = getExprType(man, log, bin.right.get(), bnt);
    bin.definition = lkp.lookupFunc(sym::Name(opName(bin.oper)), {left, right}, bin.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::UnaryExpr &un) override {
    const sym::ExprType type = getExprType(man, log, un.expr.get(), bnt);
    un.definition = lkp.lookupFunc(sym::Name(opName(un.oper)), {type}, un.loc);
    etype = lkp.getExprType();
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
    etype = call.definition->ret;
  }
  void visit(ast::MemberIdent &mem) override {
    lkp.member(sym::Name(mem.member));
    mem.object->accept(*this);
    mem.definition = lkp.lookupMember(mem.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::InitCall &init) override {
    etype.type = tlk.lookupType(init.type);
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExprType(etype);
    // @TODO lookup init function
    //init.definition = lkp.lookupFunc(argTypes(init.args), init.loc);
  }
  void visit(ast::Identifier &id) override {
    id.definition = lkp.lookupIdent(sym::Name(id.name), id.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::Self &self) override {
    self.definition = lkp.lookupSelf(self.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::Ternary &tern) override {
    tern.cond->accept(*this);
    const sym::ExprType cond = etype;
    if (cond.type != bnt.Bool) {
      log.error(tern.loc) << "Condition of ternary conditional must be a Bool" << fatal;
    }
    tern.tru->accept(*this);
    const sym::ExprType tru = etype;
    tern.fals->accept(*this);
    const sym::ExprType fals = etype;
    if (tru.type != fals.type) {
      log.error(tern.loc) << "True and false branch of ternary condition must have same type" << fatal;
    }
    etype.type = tru.type;
    etype.mut = sym::common(tru.mut, fals.mut);
    etype.ref = sym::common(tru.ref, fals.ref);
    lkp.setExprType(etype);
  }
  
  void visit(ast::StringLiteral &) override {
    etype.type = bnt.String;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExprType(etype);
  }
  void visit(ast::CharLiteral &) override {
    etype.type = bnt.Char;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExprType(etype);
  }
  void visit(ast::NumberLiteral &n) override {
    const NumberVariant num = parseNumberLiteral(n.value, log);
    if (num.type == NumberVariant::Float) {
      etype.type = bnt.Double;
    } else if (num.type == NumberVariant::Int) {
      etype.type = bnt.Int64;
    } else if (num.type == NumberVariant::UInt) {
      etype.type = bnt.UInt64;
    }
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExprType(etype);
  }
  void visit(ast::BoolLiteral &) override {
    etype.type = bnt.Bool;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExprType(etype);
  }
  void visit(ast::ArrayLiteral &) override {}
  void visit(ast::MapLiteral &) override {}
  void visit(ast::Lambda &) override {}
  
  sym::ExprType etype = sym::null_type;

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
  expr->accept(visitor);
  return visitor.etype;
}
