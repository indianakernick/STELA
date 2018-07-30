//
//  traverse.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "traverse.hpp"

#include "infer type.hpp"
#include "symbol manager.hpp"

using namespace stela;

namespace {

class FunctionInserter {
public:
  FunctionInserter(SymbolMan &man, Log &log)
    : man{man}, log{log} {}

  void enterFunc(const ast::Func &func) {
    std::unique_ptr<sym::Func> funcSym = makeFunc(func);
    funcSym->params = man.funcParams(func.params);
    inferRetType(*funcSym, func);
    sym::Func *funcSymPtr = man.insert(sym::Name(func.name), std::move(funcSym));
    man.enterScope(sym::ScopeType::function);
    insertFuncParams(*funcSymPtr, func);
  }
  void enterMemFunc(const ast::Func &func, sym::StructType *structType) {
    std::unique_ptr<sym::Func> funcSym = makeFunc(func);
    funcSym->params = man.funcParams(func.params);
    funcSym->params.insert(funcSym->params.begin(), selfType(structType));
    inferRetType(*funcSym, func);
    sym::Func *funcSymPtr = man.insert(sym::Name(func.name), std::move(funcSym));
    man.enterScope(sym::ScopeType::function);
    insertSelfParam(*funcSymPtr, structType);
    insertFuncParams(*funcSymPtr, func);
  }
  void leaveFunc() {
    man.leaveScope();
  }

private:
  SymbolMan &man;
  Log &log;

  std::unique_ptr<sym::Func> makeFunc(const ast::Func &func) {
    auto funcSym = std::make_unique<sym::Func>();
    funcSym->loc = func.loc;
    return funcSym;
  }
  void inferRetType(sym::Func &funcSym, const ast::Func &func) {
    if (func.ret) {
      funcSym.ret = {man.type(func.ret), sym::ValueCat::rvalue};
    } else {
      // @TODO infer return type
      log.warn(func.loc) << "Return type is Void by default" << endlog;
      funcSym.ret = {man.lookup("Void", func.loc), sym::ValueCat::rvalue};
    }
  }
  
  sym::ExprType selfType(sym::StructType *const structType) {
    return {structType, sym::ValueCat::lvalue_var};
  }

  void insertFuncParams(sym::Func &funcSym, const ast::Func &func) {
    for (size_t p = 0; p != func.params.size(); ++p) {
      auto *paramSym = man.insert<sym::Object>(func.params[p]);
      paramSym->etype = funcSym.params[p];
    }
  }
  void insertSelfParam(sym::Func &funcSym, sym::StructType *const structType) {
    auto *selfSym = man.insert<sym::Object>(sym::Name("self"));
    selfSym->loc = funcSym.loc;
    selfSym->etype = selfType(structType);
  }
};

class MemberScope final : public ast::Visitor {
public:
  MemberScope(Log &log, ast::MemScope astScope)
    : log{log}, astScope{astScope} {}

  void visit(ast::Func &) {
    scope = convert(astScope);
  }
  void visit(ast::Var &) {
    scope = convert(astScope);
  }
  void visit(ast::Let &) {
    scope = convert(astScope);
  }
  void visit(ast::TypeAlias &alias) {
    if (astScope == ast::MemScope::static_) {
      log.warn(alias.loc) << "No need to mark typealias as static" << endlog;
    }
    scope = sym::MemScope::static_;
  }
  void visit(ast::Init &init) {
    if (astScope == ast::MemScope::static_) {
      log.ferror(init.loc) << "Cannot mark init as static" << endlog;
    }
    scope = sym::MemScope::instance;
  }
  void visit(ast::Struct &strt) {
    if (astScope == ast::MemScope::static_) {
      log.warn(strt.loc) << "No need to mark struct as static" << endlog;
    }
    scope = sym::MemScope::static_;
  }
  void visit(ast::Enum &enm) {
    if (astScope == ast::MemScope::static_) {
      log.warn(enm.loc) << "No need to mark enum as static" << endlog;
    }
    scope = sym::MemScope::static_;
  }

  sym::MemScope scope;

private:
  Log &log;
  const ast::MemScope astScope;
  
  static sym::MemScope convert(const ast::MemScope scope) {
    if (scope == ast::MemScope::member) {
      return sym::MemScope::instance;
    } else {
      return sym::MemScope::static_;
    }
  }
};

sym::MemScope memScope(const ast::Member &mem, Log &log) {
  MemberScope visitor{log, mem.scope};
  mem.node->accept(visitor);
  return visitor.scope;
}

sym::MemAccess memAccess(const ast::Member &mem, Log &) {
  // @TODO maybe make vars private by default and functions public by default?
  if (mem.access == ast::MemAccess::private_) {
    return sym::MemAccess::private_;
  } else {
    return sym::MemAccess::public_;
  }
}

class Visitor final : public ast::Visitor {
public:
  Visitor(sym::Scopes &scopes, Log &log)
    : man{scopes, log}, log{log} {}
  
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
    const ast::TypePtr &type,
    const ast::ExprPtr &expr,
    const Loc loc
  ) {
    sym::Symbol *exprType = expr ? exprFunc(man, expr.get()).type : nullptr;
    sym::Symbol *symType = type ? man.type(type) : exprType;
    if (exprType != nullptr && exprType != symType) {
      log.ferror(loc) << "Expression and declaration type do not match" << endlog;
    }
    return symType;
  }
  void visit(ast::Var &var) override {
    auto *varSym = man.insert<sym::Object>(var);
    varSym->etype.cat = sym::ValueCat::lvalue_var;
    varSym->etype.type = objectType(var.type, var.expr, var.loc);
  }
  void visit(ast::Let &let) override {
    auto *letSym = man.insert<sym::Object>(let);
    letSym->etype.cat = sym::ValueCat::lvalue_let;
    letSym->etype.type = objectType(let.type, let.expr, let.loc);
  }
  void visit(ast::TypeAlias &alias) override {
    auto *aliasSym = man.insert<sym::TypeAlias>(alias);
    aliasSym->type = man.type(alias.type);
  }
  
  void visit(ast::Init &) override {}
  void visit(ast::Struct &strut) override {
    auto *structSym = man.insert<sym::StructType>(strut);
    structSym->scope = man.enterScope(sym::ScopeType::structure);
    parentStruct = structSym;
    for (const ast::Member &mem : strut.body) {
      //memAccess(mem, log);
      //memScope(mem, log);
    }
    man.leaveScope();
  }
  void visit(ast::Enum &num) override {
    auto *enumSym = man.insert<sym::EnumType>(num);
    enumSym->loc = num.loc;
    enumSym->scope = man.enterScope(sym::ScopeType::enumeration);
    for (const ast::EnumCase &cs : num.cases) {
      auto caseSym = std::make_unique<sym::Object>();
      caseSym->loc = cs.loc;
      caseSym->etype.type = enumSym;
      caseSym->etype.cat = sym::ValueCat::lvalue_let;
      man.insert(sym::Name(cs.name), std::move(caseSym));
    }
    man.leaveScope();
  }

private:
  SymbolMan man;
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
