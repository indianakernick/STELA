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
    if (expr && exprType.cat != sym::ExprCat::expr) {
      log.error(loc) << "Cannot initialize variable with a type" << fatal;
    }
    sym::Symbol *symType = type ? tlk.lookupType(type) : exprType.type;
    if (exprType.type != nullptr && exprType.type != symType) {
      log.error(loc) << "Expression and declaration type do not match" << fatal;
    }
    return symType;
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
    aliasSym->type = tlk.lookupType(alias.type);
  }
  
  void visit(ast::Init &init) override {
    auto *strutIns = dynamic_cast<StructInserter *>(ins.get());
    if (strutIns == nullptr) {
      log.error(init.loc) << "Init function must be member of a struct" << fatal;
    }
    sym::Func *const funcSym = strutIns->insert(init);
    funcSym->scope = man.enterScope<sym::FuncScope>();
    strutIns->enterFuncScope(funcSym, init);
    FuncInserter inserter{funcSym->scope, log};
    SymbolInserter *const old = ins.set(&inserter);
    for (const ast::StatPtr &stat : init.body.nodes) {
      visitStat(stat);
    }
    ins.restore(old);
    man.leaveScope();
  }
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
      if (cs.value) {
        const sym::ExprType type = getExprType(man, log, cs.value.get(), bnt);
        if (type.cat != sym::ExprCat::expr) {
          log.error(cs.loc) << "Enum case \"" << cs.name
            << "\" cannot be a type" << fatal;
        }
        if (type.type != bnt.Int64 && type.type != enumSym) {
          log.error(cs.loc) << "Enum case \"" << cs.name
            << "\" must have value of Int or another case" << fatal;
        }
      }
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
