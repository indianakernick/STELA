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
#include "scope manager.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(sym::Scopes &scopes, Log &log, const BuiltinTypes &types)
    : man{scopes}, log{log}, ins{man.global(), log, types}, tlk{man, log}, bnt{types} {}
  
  void visitStat(const ast::StatPtr &stat) {
    if (ast::Expression *expr = dynamic_cast<ast::Expression *>(stat.get())) {
      getExprType(man, log, expr, bnt);
    } else {
      stat->accept(*this);
    }
  }
  
  void visit(ast::Block &block) override {
    BlockInserter inserter(man.enterScope<sym::BlockScope>(), log);
    SymbolInserter *const old = ins.set(&inserter);
    for (const ast::StatPtr &stat : block.nodes) {
      visitStat(stat);
    }
    ins.restore(old);
    man.leaveScope();
  }

  void visit(ast::Return &ret) override {
    if (ret.expr) {
      getExprType(man, log, ret.expr.get(), bnt);
    }
  }

  void visit(ast::Func &func) override {
    sym::Func *const funcSym = ins.insert(func);
    funcSym->scope = man.enterScope<sym::FuncScope>();
    ins.enterFuncScope(funcSym, func);
    FuncInserter inserter{funcSym->scope, log};
    SymbolInserter *const old = ins.set(&inserter);
    for (const ast::StatPtr &stat : func.body.nodes) {
      visitStat(stat);
    }
    ins.restore(old);
    man.leaveScope();
  }
  sym::Symbol *objectType(
    const ast::TypePtr &type,
    const ast::ExprPtr &expr,
    const Loc loc
  ) {
    sym::ExprType exprType = expr ? getExprType(man, log, expr.get(), bnt) : sym::ExprType{};
    if (expr && exprType.typeExpr) {
      log.error(loc) << "Cannot initialize variable with a type" << fatal;
    }
    sym::Symbol *symType = type ? tlk.lookupType(type) : exprType.type;
    if (exprType.type != nullptr && exprType.type != symType) {
      log.error(loc) << "Expression and declaration type do not match" << fatal;
    }
    return symType;
  }
  void visit(ast::Var &var) override {
    auto *varSym = ins.insert<sym::Object>(var);
    varSym->etype.type = objectType(var.type, var.expr, var.loc);
    varSym->etype.mut = sym::ValueMut::var;
    varSym->etype.ref = sym::ValueRef::val;
  }
  void visit(ast::Let &let) override {
    auto *letSym = ins.insert<sym::Object>(let);
    letSym->etype.type = objectType(let.type, let.expr, let.loc);
    letSym->etype.mut = sym::ValueMut::let;
    letSym->etype.ref = sym::ValueRef::val;
  }
  void visit(ast::TypeAlias &alias) override {
    auto *aliasSym = ins.insert<sym::TypeAlias>(alias);
    aliasSym->type = tlk.lookupType(alias.type);
  }
  
  void visit(ast::Init &) override {}
  void visit(ast::Struct &strut) override {
    auto *structSym = ins.insert<sym::StructType>(strut);
    structSym->scope = man.enterScope<sym::StructScope>();
    StructInserter inserter{structSym, log};
    SymbolInserter *const old = ins.set(&inserter);
    for (const ast::Member &mem : strut.body) {
      inserter.accessScope(mem);
      mem.node->accept(*this);
    }
    ins.restore(old);
    man.leaveScope();
  }
  void visit(ast::Enum &num) override {
    auto *enumSym = ins.insert<sym::EnumType>(num);
    enumSym->scope = man.enterScope<sym::EnumScope>();
    EnumInserter inserter{enumSym, log};
    for (const ast::EnumCase &cs : num.cases) {
      inserter.insert(cs);
    }
    man.leaveScope();
  }

private:
  ScopeMan man;
  Log &log;
  InserterManager ins;
  TypeLookup tlk;
  const BuiltinTypes &bnt;
};

}

void stela::traverse(sym::Scopes &scopes, const AST &ast, Log &log, const BuiltinTypes &types) {
  Visitor visitor{scopes, log, types};
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(visitor);
  }
}
