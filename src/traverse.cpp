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
#include "member access scope.hpp"

using namespace stela;

namespace {

class FunctionInserter {
public:
  FunctionInserter(ScopeMan &man, Log &log)
    : man{man}, log{log} {}

  void enterFunc(const ast::Func &func) {
    std::unique_ptr<sym::Func> funcSym = makeFunc(func);
    funcSym->params = funcParams(man.current(), log, func.params);
    inferRetType(*funcSym, func);
    sym::Func *funcSymPtr = insert(man.current(), log, sym::Name(func.name), std::move(funcSym));
    man.enterScope<sym::FuncScope>();
    insertFuncParams(*funcSymPtr, func);
  }
  void enterMemFunc(const ast::Func &func, sym::StructType *structType) {
    std::unique_ptr<sym::Func> funcSym = makeFunc(func);
    funcSym->params = funcParams(man.current(), log, func.params);
    funcSym->params.insert(funcSym->params.begin(), selfType(structType));
    inferRetType(*funcSym, func);
    sym::Func *funcSymPtr = insert(man.current(), log, sym::Name(func.name), std::move(funcSym));
    man.enterScope<sym::FuncScope>();
    insertSelfParam(*funcSymPtr, structType);
    insertFuncParams(*funcSymPtr, func);
  }
  void leaveFunc() {
    man.leaveScope();
  }

private:
  ScopeMan &man;
  Log &log;

  std::unique_ptr<sym::Func> makeFunc(const ast::Func &func) {
    auto funcSym = std::make_unique<sym::Func>();
    funcSym->loc = func.loc;
    return funcSym;
  }
  void inferRetType(sym::Func &funcSym, const ast::Func &func) {
    if (func.ret) {
      funcSym.ret = {type(man.current(), log, func.ret), sym::ValueCat::rvalue};
    } else {
      // @TODO infer return type
      log.warn(func.loc) << "Return type is Void by default" << endlog;
      funcSym.ret = {lookup(man.current(), log, "Void", func.loc), sym::ValueCat::rvalue};
    }
  }
  
  sym::ExprType selfType(sym::StructType *const structType) {
    return {structType, sym::ValueCat::lvalue_var};
  }

  void insertFuncParams(sym::Func &funcSym, const ast::Func &func) {
    for (size_t p = 0; p != func.params.size(); ++p) {
      auto *paramSym = insert<sym::Object>(man.current(), log, func.params[p]);
      paramSym->etype = funcSym.params[p];
    }
  }
  void insertSelfParam(sym::Func &funcSym, sym::StructType *const structType) {
    auto *selfSym = insert<sym::Object>(man.current(), log, sym::Name("self"));
    selfSym->loc = funcSym.loc;
    selfSym->etype = selfType(structType);
  }
};

class Visitor final : public ast::Visitor {
public:
  Visitor(sym::Scopes &scopes, Log &log)
    : man{scopes}, log{log} {}
  
  void visit(ast::Block &block) override {
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
  }

  void visit(ast::Func &func) override {
    FunctionInserter funInsert{man, log};
    funInsert.enterFunc(func);
    visit(func.body);
    funInsert.leaveFunc();
  }
  sym::Symbol *objectType(
    const ast::TypePtr &atype,
    const ast::ExprPtr &expr,
    const Loc loc
  ) {
    sym::Symbol *exprType = expr ? exprFunc(man, log, expr.get()).type : nullptr;
    sym::Symbol *symType = atype ? type(man.current(), log, atype) : exprType;
    if (exprType != nullptr && exprType != symType) {
      log.ferror(loc) << "Expression and declaration type do not match" << endlog;
    }
    return symType;
  }
  void visit(ast::Var &var) override {
    auto *varSym = insert<sym::Object>(man.current(), log, var);
    varSym->etype.cat = sym::ValueCat::lvalue_var;
    varSym->etype.type = objectType(var.type, var.expr, var.loc);
  }
  void visit(ast::Let &let) override {
    auto *letSym = insert<sym::Object>(man.current(), log, let);
    letSym->etype.cat = sym::ValueCat::lvalue_let;
    letSym->etype.type = objectType(let.type, let.expr, let.loc);
  }
  void visit(ast::TypeAlias &alias) override {
    auto *aliasSym = insert<sym::TypeAlias>(man.current(), log, alias);
    aliasSym->type = type(man.current(), log, alias.type);
  }
  
  void visit(ast::Init &) override {}
  void visit(ast::Struct &strut) override {
    auto *structSym = insert<sym::StructType>(man.current(), log, strut);
    structSym->scope = man.enterScope<sym::StructScope>();
    parentStruct = structSym;
    for (const ast::Member &mem : strut.body) {
      //memAccess(mem, log);
      //memScope(mem, log);
    }
    man.leaveScope();
  }
  void visit(ast::Enum &num) override {
    auto *enumSym = insert<sym::EnumType>(man.current(), log, num);
    enumSym->loc = num.loc;
    enumSym->scope = man.enterScope<sym::EnumScope>();
    for (const ast::EnumCase &cs : num.cases) {
      auto caseSym = std::make_unique<sym::Object>();
      caseSym->loc = cs.loc;
      caseSym->etype.type = enumSym;
      caseSym->etype.cat = sym::ValueCat::lvalue_let;
      insert(man.current(), log, sym::Name(cs.name), std::move(caseSym));
    }
    man.leaveScope();
  }

private:
  ScopeMan man;
  Log &log;
  sym::StructType *parentStruct = nullptr;
};

}

void stela::traverse(sym::Scopes &scopes, const AST &ast, Log &log) {
  Visitor visitor{scopes, log};
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(visitor);
  }
}
