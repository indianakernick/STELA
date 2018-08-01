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
  Visitor(sym::Scopes &scopes, Log &log)
    : man{scopes}, log{log}, ins{dynamic_cast<sym::NSScope *>(man.cur()), log} {}
  
  void visit(ast::Block &block) override {
    BlockInserter inserter(man.enterScope<sym::BlockScope>(), log);
    SymbolInserter *const old = ins.set(&inserter);
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
    ins.restore(old);
    man.leaveScope();
  }

  void visit(ast::Func &func) override {
    sym::Func *const funcSym = ins.insert(func);
    funcSym->scope = man.enterScope<sym::FuncScope>();
    ins.enterFuncScope(funcSym, func);
    FuncInserter inserter{funcSym->scope, log};
    SymbolInserter *const old = ins.set(&inserter);
    for (const ast::StatPtr &stat : func.body.nodes) {
      stat->accept(*this);
    }
    ins.restore(old);
    man.leaveScope();
  }
  sym::Symbol *objectType(
    const ast::TypePtr &atype,
    const ast::ExprPtr &expr,
    const Loc loc
  ) {
    sym::Symbol *exprType = expr ? exprFunc(man, log, expr.get()).type : nullptr;
    sym::Symbol *symType = atype ? type(man.cur(), log, atype) : exprType;
    if (exprType != nullptr && exprType != symType) {
      log.ferror(loc) << "Expression and declaration type do not match" << endlog;
    }
    return symType;
  }
  void visit(ast::Var &var) override {
    auto *varSym = ins.insert<sym::Object>(var);
    varSym->etype.cat = sym::ValueCat::lvalue_var;
    varSym->etype.type = objectType(var.type, var.expr, var.loc);
  }
  void visit(ast::Let &let) override {
    auto *letSym = ins.insert<sym::Object>(let);
    letSym->etype.cat = sym::ValueCat::lvalue_let;
    letSym->etype.type = objectType(let.type, let.expr, let.loc);
  }
  void visit(ast::TypeAlias &alias) override {
    auto *aliasSym = ins.insert<sym::TypeAlias>(alias);
    aliasSym->type = type(man.cur(), log, alias.type);
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
};

}

void stela::traverse(sym::Scopes &scopes, const AST &ast, Log &log) {
  Visitor visitor{scopes, log};
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(visitor);
  }
}
