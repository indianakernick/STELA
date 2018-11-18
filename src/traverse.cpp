//
//  traverse.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "traverse.hpp"

#include "infer type.hpp"
#include "scope insert.hpp"
#include "scope lookup.hpp"
#include "operator name.hpp"
#include "compare types.hpp"
#include "scope manager.hpp"
#include "scope traverse.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(ScopeMan &man, Symbols &syms, Log &log)
    : man{man},
      log{log},
      ins{syms.modules, man, log},
      tlk{syms.modules, man, log},
      syms{syms} {}
  
  void visitExpr(const ast::ExprPtr &expr) {
    if (expr) {
      getExprType(syms, man, log, expr.get());
    }
  }
  void visitCond(const ast::ExprPtr &expr) {
    assert(expr);
    const sym::ExprType etype = getExprType(syms, man, log, expr.get());
    if (!compareTypes(tlk, etype.type, syms.builtins.Bool)) {
      log.error(expr->loc) << "Condition expression must be of type Bool" << fatal;
    }
  }
  
  void visit(ast::Block &block) override {
    man.enterScope(sym::Scope::Type::block);
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
    man.leaveScope();
  }
  void visit(ast::If &fi) override {
    man.enterScope(sym::Scope::Type::block);
    visitCond(fi.cond);
    fi.body->accept(*this);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
    man.leaveScope();
  }
  void visit(ast::Switch &swich) override {
    const sym::ExprType etype = getExprType(syms, man, log, swich.expr.get());
    bool foundDef = false;
    for (const ast::SwitchCase &cs : swich.cases) {
      if (cs.expr) {
        const sym::ExprType caseType = getExprType(syms, man, log, cs.expr.get());
        if (!compareTypes(tlk, caseType.type, etype.type)) {
          log.error(cs.loc) << "Case expression type doesn't match type of switch expression" << fatal;
        }
      } else {
        if (foundDef) {
          log.error(cs.loc) << "Multiple default cases found in switch" << fatal;
        }
        foundDef = true;
      }
      man.enterScope(sym::Scope::Type::flow);
      cs.body->accept(*this);
      man.leaveScope();
    }
  }
  void checkFlowKeyword(const std::string_view keyword, const Loc loc) {
    sym::Scope *const flow = findNearestNot(sym::Scope::Type::block, man.cur());
    if (flow->type != sym::Scope::Type::flow) {
      log.error(loc) << "Invalid usage of keyword \"" << keyword
        << "\" outside of loop or switch" << fatal;
    }
  }
  void visit(ast::Break &brake) override {
    checkFlowKeyword("break", brake.loc);
  }
  void visit(ast::Continue &continu) override {
    checkFlowKeyword("continue", continu.loc);
  }
  void visit(ast::Return &ret) override {
    visitExpr(ret.expr);
  }
  void visit(ast::While &wile) override {
    man.enterScope(sym::Scope::Type::flow);
    visitCond(wile.cond);
    wile.body->accept(*this);
    man.leaveScope();
  }
  void visit(ast::For &four) override {
    man.enterScope(sym::Scope::Type::flow);
    four.init->accept(*this);
    visitCond(four.cond);
    four.incr->accept(*this);
    four.body->accept(*this);
    man.leaveScope();
  }

  void visit(ast::Func &func) override {
    sym::Func *const funcSym = ins.insert(func);
    funcSym->scope = man.enterScope(sym::Scope::Type::func);
    ins.enterFuncScope(funcSym, func);
    for (const ast::StatPtr &stat : func.body.nodes) {
      stat->accept(*this);
    }
    man.leaveScope();
  }
  ast::TypePtr objectType(
    const ast::TypePtr &type,
    const ast::ExprPtr &expr,
    const Loc loc
  ) {
    sym::ExprType exprType = expr ? getExprType(syms, man, log, expr.get()) : sym::null_type;
    if (type) {
      tlk.validateType(type);
    }
    if (expr && !exprType.type) {
      log.error(loc) << "Cannot initialize variable with void expression" << fatal;
    }
    if (type && exprType.type && !compareTypes(tlk, type, exprType.type)) {
      log.error(loc) << "Expression and declaration type do not match" << fatal;
    }
    if (type) {
      return type;
    } else {
      return exprType.type;
    }
  }
  void visit(ast::Var &var) override {
    sym::ExprType etype;
    etype.type = objectType(var.type, var.expr, var.loc);
    etype.mut = sym::ValueMut::var;
    etype.ref = sym::ValueRef::val;
    auto *varSym = ins.insert<sym::Object>(var);
    varSym->etype = etype;
  }
  void visit(ast::Let &let) override {
    sym::ExprType etype;
    etype.type = objectType(let.type, let.expr, let.loc);
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    auto *letSym = ins.insert<sym::Object>(let);
    letSym->etype = etype;
  }
  void visit(ast::TypeAlias &alias) override {
    auto *aliasSym = ins.insert<sym::TypeAlias>(alias);
    aliasSym->node = {retain, &alias};
    tlk.validateType(alias.type);
  }
  
  void visit(ast::CompAssign &as) override {
    const sym::ExprType left = getExprType(syms, man, log, as.left.get());
    const sym::ExprType right = getExprType(syms, man, log, as.right.get());
    if (auto builtinLeft = tlk.lookupConcrete<ast::BuiltinType>(left.type)) {
      if (auto builtinRight = tlk.lookupConcrete<ast::BuiltinType>(right.type)) {
        if (validOp(as.oper, builtinLeft, builtinRight)) {
          if (left.mut == sym::ValueMut::var) {
            return;
          } else {
            log.error(as.loc) << "Left side of compound assignment must be mutable" << fatal;
          }
        }
      }
    }
    log.error(as.loc) << "Invalid operands to compound assignment operator " << opName(as.oper) << fatal;
  }
  void visit(ast::IncrDecr &as) override {
    const sym::ExprType etype = getExprType(syms, man, log, as.expr.get());
    if (auto builtin = tlk.lookupConcrete<ast::BuiltinType>(etype.type)) {
      if (validIncr(builtin)) {
        return;
      }
    }
    log.error(as.loc) << "Invalid operand to unary operator " << (as.incr ? "++" : "--") << fatal;
  }
  void visit(ast::Assign &as) override {
    const sym::ExprType left = getExprType(syms, man, log, as.left.get());
    const sym::ExprType right = getExprType(syms, man, log, as.right.get());
    if (!compareTypes(tlk, left.type, right.type)) {
      log.error(as.loc) << "Assignment types do not match" << fatal;
    }
    if (left.mut == sym::ValueMut::let) {
      log.error(as.loc) << "Left side of assignment must be mutable" << fatal;
    }
  }
  void visit(ast::DeclAssign &as) override {
    sym::ExprType etype;
    etype.type = objectType(nullptr, as.expr, as.loc);
    etype.mut = sym::ValueMut::var;
    etype.ref = sym::ValueRef::val;
    auto *varSym = ins.insert<sym::Object>(as);
    varSym->etype = etype;
  }
  void visit(ast::CallAssign &as) override {
    const sym::ExprType etype = getExprType(syms, man, log, &as.call);
    if (etype.type) {
      log.error(as.loc) << "Discarded return value" << fatal;
    }
  }

private:
  ScopeMan &man;
  Log &log;
  InserterManager ins;
  NameLookup tlk;
  Symbols &syms;
};

}

void stela::traverse(ScopeMan &man, sym::Module &module, Symbols &syms, Log &log) {
  Visitor visitor{man, syms, log};
  for (const ast::DeclPtr &decl : module.decls) {
    decl->accept(visitor);
  }
}
