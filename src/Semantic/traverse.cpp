//
//  traverse.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "traverse.hpp"

#include "infer type.hpp"
#include "symbol desc.hpp"
#include "scope insert.hpp"
#include "scope lookup.hpp"
#include "operator name.hpp"
#include "compare types.hpp"
#include "scope traverse.hpp"
#include "builtin symbols.hpp"
#include "Utils/unreachable.hpp"
#include "check missing return.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(sym::Ctx ctx)
    : ctx{ctx} {}
  
  void visitCond(const ast::ExprPtr &expr) {
    assert(expr);
    getExprType(ctx, expr, ctx.btn.Bool);
  }
  
  void visit(ast::Block &block) override {
    ctx.man.enterScope(sym::ScopeType::block);
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
    ctx.man.leaveScope();
  }
  void visit(ast::If &fi) override {
    ctx.man.enterScope(sym::ScopeType::block);
    visitCond(fi.cond);
    fi.body->accept(*this);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
    ctx.man.leaveScope();
  }
  void visit(ast::Switch &swich) override {
    const sym::ExprType etype = getExprType(ctx, swich.expr, nullptr);
    bool foundDef = false;
    for (ast::SwitchCase &cs : swich.cases) {
      cs.parent = &swich;
      if (cs.expr) {
        getExprType(ctx, cs.expr, etype.type);
      } else {
        if (foundDef) {
          ctx.log.error(cs.loc) << "Multiple default cases found in switch" << fatal;
        }
        foundDef = true;
      }
      ctx.man.enterScope(sym::ScopeType::flow);
      cs.body->accept(*this);
      ctx.man.leaveScope();
    }
  }
  void checkFlowKeyword(const std::string_view keyword, const Loc loc) {
    sym::Scope *const flow = findNearestNot(sym::ScopeType::block, ctx.man.cur());
    if (flow->type != sym::ScopeType::flow) {
      ctx.log.error(loc) << "Invalid usage of keyword \"" << keyword
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
    sym::Scope *funcScope = findNearestEither(
      sym::ScopeType::func,
      sym::ScopeType::closure,
      ctx.man.cur()
    );
    assert(funcScope->symbol);
    ast::TypePtr *retType;
    if (auto func = dynamic_cast<sym::Func *>(funcScope->symbol)) {
      retType = &func->ret.type;
    } else if (auto lamb = dynamic_cast<sym::Lambda *>(funcScope->symbol)) {
      retType = &lamb->ret.type;
    } else {
      UNREACHABLE();
    }
    if (ret.expr) {
      if (*retType) {
        getExprType(ctx, ret.expr, *retType);
      } else {
        *retType = getExprType(ctx, ret.expr, nullptr).type;
      }
    } else {
      if (*retType) {
        if (!compareTypes(ctx, *retType, ctx.btn.Void)) {
          ctx.log.error(ret.loc) << "Expected " << typeDesc(*retType) << " but got void" << fatal;
        }
      } else {
        *retType = ctx.btn.Void;
      }
    }
  }
  void visit(ast::While &wile) override {
    ctx.man.enterScope(sym::ScopeType::flow);
    visitCond(wile.cond);
    wile.body->accept(*this);
    ctx.man.leaveScope();
  }
  void visit(ast::For &four) override {
    ctx.man.enterScope(sym::ScopeType::flow);
    if (four.init) {
      four.init->accept(*this);
    }
    visitCond(four.cond);
    if (four.incr) {
      four.incr->accept(*this);
    }
    four.body->accept(*this);
    ctx.man.leaveScope();
  }

  void visit(ast::Func &func) override {
    sym::Func *const funcSym = insert(ctx, func);
    funcSym->scope = ctx.man.enterScope(sym::ScopeType::func, funcSym);
    enterFuncScope(funcSym, func);
    for (const ast::StatPtr &stat : func.body.nodes) {
      stat->accept(*this);
    }
    leaveFuncScope(ctx, funcSym, func);
    ctx.man.leaveScope();
    checkMissingRet(ctx, func.body, func.ret, func.loc);
  }
  void visit(ast::ExtFunc &func) override {
    insert(ctx, func);
  }
  ast::TypePtr objectType(
    const ast::TypePtr &type,
    const ast::ExprPtr &expr,
    const Loc loc
  ) {
    if (type) {
      validateType(ctx, type);
    }
    sym::ExprType exprType = expr ? getExprType(ctx, expr, type) : sym::null_type;
    if (compareTypes(ctx, exprType.type, ctx.btn.Void)) {
      ctx.log.error(loc) << "Cannot initialize variable with void expression" << fatal;
    }
    if (!type && !expr) {
      ctx.log.error(loc) << "Cannot infer type of variable" << fatal;
    }
    if (type) {
      return type;
    } else {
      return exprType.type;
    }
  }
  void visit(ast::Var &var) override {
    sym::ExprType etype = sym::makeVarVal(objectType(var.type, var.expr, var.loc));
    auto *varSym = insert<sym::Object>(ctx, var);
    varSym->scope = ctx.man.cur();
    varSym->etype = std::move(etype);
  }
  void visit(ast::Let &let) override {
    sym::ExprType etype = sym::makeLetVal(objectType(let.type, let.expr, let.loc));
    auto *letSym = insert<sym::Object>(ctx, let);
    letSym->scope = ctx.man.cur();
    letSym->etype = std::move(etype);
  }
  void visit(ast::TypeAlias &alias) override {
    validateType(ctx, alias.type);
    auto *aliasSym = insert<sym::TypeAlias>(ctx, alias);
    aliasSym->scope = ctx.man.cur();
    aliasSym->node = {retain, &alias};
  }
  
  void visit(ast::CompAssign &as) override {
    const sym::ExprType dst = getExprType(ctx, as.dst, nullptr);
    const sym::ExprType src = getExprType(ctx, as.src, dst.type);
    if (auto builtinLeft = lookupConcrete<ast::BtnType>(ctx, dst.type)) {
      if (auto builtinRight = lookupConcrete<ast::BtnType>(ctx, src.type)) {
        assert(builtinLeft == builtinRight);
        if (validOp(as.oper, builtinLeft)) {
          if (dst.mut == sym::ValueMut::var) {
            return;
          } else {
            ctx.log.error(as.loc) << "Left side of compound assignment must be mutable" << fatal;
          }
        }
      }
    }
    ctx.log.error(as.loc) << "Invalid operands to compound assignment operator " << opName(as.oper) << fatal;
  }
  void visit(ast::IncrDecr &as) override {
    const sym::ExprType etype = getExprType(ctx, as.expr, nullptr);
    if (etype.mut == sym::ValueMut::let) {
      ctx.log.error(as.loc) << "Operand to mutating unary operator must be mutable" << fatal;
    }
    if (auto builtin = lookupConcrete<ast::BtnType>(ctx, etype.type)) {
      if (validIncr(builtin)) {
        return;
      }
    }
    ctx.log.error(as.loc) << "Invalid operand to unary operator " << (as.incr ? "++" : "--") << fatal;
  }
  void visit(ast::Assign &as) override {
    const sym::ExprType dst = getExprType(ctx, as.dst, nullptr);
    const sym::ExprType src = getExprType(ctx, as.src, dst.type);
    if (dst.mut == sym::ValueMut::let) {
      ctx.log.error(as.loc) << "Left side of assignment must be mutable" << fatal;
    }
  }
  void visit(ast::DeclAssign &as) override {
    sym::ExprType etype = sym::makeVarVal(objectType(nullptr, as.expr, as.loc));
    auto *varSym = insert<sym::Object>(ctx, as);
    varSym->scope = ctx.man.cur();
    varSym->etype = etype;
  }
  void visit(ast::CallAssign &as) override {
    const sym::ExprType etype = getExprType(ctx, {retain, &as.call}, nullptr);
    if (!compareTypes(ctx, etype.type, ctx.btn.Void)) {
      ctx.log.error(as.loc) << "Discarded return value" << fatal;
    }
  }

private:
  sym::Ctx ctx;
};

}

void stela::traverse(sym::Ctx ctx, const ast::Decls &decls) {
  Visitor visitor{ctx};
  for (const ast::DeclPtr &decl : decls) {
    decl->accept(visitor);
  }
}

void stela::traverse(sym::Ctx ctx, const ast::Block &block) {
  Visitor visitor{ctx};
  for (const ast::StatPtr &stat : block.nodes) {
    stat->accept(visitor);
  }
}
