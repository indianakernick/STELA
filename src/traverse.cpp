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
    funcSym->params = funcParams(man.cur(), log, func.params);
    inferRetType(*funcSym, func);
    sym::Func *funcSymPtr = insert(man.cur(), log, sym::Name(func.name), std::move(funcSym));
    man.enterScope<sym::FuncScope>();
    insertFuncParams(*funcSymPtr, func);
  }
  void enterMemFunc(const ast::Func &func, sym::StructType *structType) {
    std::unique_ptr<sym::Func> funcSym = makeFunc(func);
    funcSym->params = funcParams(man.cur(), log, func.params);
    funcSym->params.insert(funcSym->params.begin(), selfType(structType));
    inferRetType(*funcSym, func);
    sym::Func *funcSymPtr = insert(man.cur(), log, sym::Name(func.name), std::move(funcSym));
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
      funcSym.ret = {type(man.cur(), log, func.ret), sym::ValueCat::rvalue};
    } else {
      // @TODO infer return type
      log.warn(func.loc) << "Return type is Void by default" << endlog;
      funcSym.ret = {lookup(man.cur(), log, "Void", func.loc), sym::ValueCat::rvalue};
    }
  }
  
  sym::ExprType selfType(sym::StructType *const structType) {
    return {structType, sym::ValueCat::lvalue_var};
  }

  void insertFuncParams(sym::Func &funcSym, const ast::Func &func) {
    for (size_t p = 0; p != func.params.size(); ++p) {
      auto *paramSym = insert<sym::Object>(man.cur(), log, func.params[p]);
      paramSym->etype = funcSym.params[p];
    }
  }
  void insertSelfParam(sym::Func &funcSym, sym::StructType *const structType) {
    auto *selfSym = insert<sym::Object>(man.cur(), log, sym::Name("self"));
    selfSym->loc = funcSym.loc;
    selfSym->etype = selfType(structType);
  }
};

class SymbolInserter {
public:
  virtual ~SymbolInserter() = default;
  
  virtual void insert(const sym::Name &, sym::SymbolPtr) = 0;
  virtual void insert(const sym::Name &, sym::FuncPtr) = 0;
};

class BasicInserter final : public SymbolInserter {
public:
  BasicInserter(sym::Scope *scope, Log &log)
    : scope{scope}, log{log} {}

  void insert(const sym::Name &name, sym::SymbolPtr symbol) override {
    ::insert(scope, log, name, std::move(symbol));
  }
  void insert(const sym::Name &name, sym::FuncPtr func) override {
    ::insert(scope, log, name, std::move(func));
  }

private:
  sym::Scope *scope;
  Log &log;
};

using NSInserter = BasicInserter;
using FuncInserter = BasicInserter;
using EnumInserter = BasicInserter;

class StructInserter final : public SymbolInserter {
public:
  StructInserter(sym::StructType *strut, Log &log)
    : strut{strut}, log{log} {}

  void insert(const sym::Name &name, sym::SymbolPtr symbol) override {
    ::insert(strut->scope, log, {name, access, scope}, std::move(symbol));
  }
  void insert(const sym::Name &name, sym::FuncPtr func) override {
    ::insert(strut->scope, log, {name, access, scope}, std::move(func));
  }

  void setAccess(const sym::MemAccess newAccess) {
    access = newAccess;
  }
  void setScope(const sym::MemScope newScope) {
    scope = newScope;
  }

private:
  sym::StructType *strut;
  Log &log;
  sym::MemAccess access;
  sym::MemScope scope;
};

class InserterManager {
public:
  InserterManager(ScopeMan &man, Log &log)
    : defIns{man.cur(), log}, ins{&defIns} {}
  
  template <typename Symbol>
  Symbol *insert(const sym::Name &name) {
    static_assert(!std::is_same_v<Symbol, sym::Func>, "Need to set function parameters when inserting function");
    assert(ins);
    auto symbol = std::make_unique<Symbol>();
    Symbol *const ret = symbol.get();
    ins->insert(name, std::move(symbol));
    return ret;
  }
  template <typename Symbol, typename AST_Node>
  Symbol *insert(const AST_Node &node) {
    Symbol *const symbol = insert<Symbol>(sym::Name(node.name));
    symbol->loc = node.loc;
    return symbol;
  }
  void insert(const sym::Name &name, sym::SymbolPtr symbol) {
    assert(ins);
    ins->insert(name, std::move(symbol));
  }
  void insert(const sym::Name &name, sym::FuncPtr func) {
    assert(ins);
    ins->insert(name, std::move(func));
  }
  
  SymbolInserter *set(SymbolInserter *const newIns) {
    assert(newIns);
    return std::exchange(ins, newIns);
  }
  SymbolInserter *setDef() {
    return set(&defIns);
  }
  void restore(SymbolInserter *const oldIns) {
    assert(oldIns);
    ins = oldIns;
  }

private:
  BasicInserter defIns;
  SymbolInserter *ins;
};

class Visitor final : public ast::Visitor {
public:
  Visitor(sym::Scopes &scopes, Log &log)
    : man{scopes}, log{log}, ins{man, log} {}
  
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
      inserter.setAccess(memAccess(mem, log));
      inserter.setScope(memScope(mem, log));
      //mem.node->accept(*this);
    }
    ins.restore(old);
    man.leaveScope();
  }
  void visit(ast::Enum &num) override {
    auto *enumSym = ins.insert<sym::EnumType>(num);
    enumSym->scope = man.enterScope<sym::EnumScope>();
    EnumInserter inserter{enumSym->scope, log};
    for (const ast::EnumCase &cs : num.cases) {
      auto caseSym = std::make_unique<sym::Object>();
      caseSym->loc = cs.loc;
      caseSym->etype.type = enumSym;
      caseSym->etype.cat = sym::ValueCat::lvalue_let;
      inserter.insert(sym::Name(cs.name), std::move(caseSym));
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
