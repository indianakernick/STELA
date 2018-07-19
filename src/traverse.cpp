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

enum class ScopeType {
  func,
  global,
  struct_member,
  struct_static
};

class ScopeTypes {
public:
  ScopeTypes() = default;

  ScopeType current() const {
    return cur;
  }
  ScopeType parent() const {
    return par;
  }
  ScopeType enterScope(const ScopeType newScope) {
    const ScopeType oldScope = par;
    par = cur;
    cur = newScope;
    return oldScope;
  }
  void leaveScope(const ScopeType oldScope) {
    cur = par;
    par = oldScope;
  }

private:
  ScopeType cur = ScopeType::global;
  ScopeType par = ScopeType::global;
};

class Visitor final : public ast::Visitor {
public:
  Visitor(sym::Scopes &scopes, Log &log)
    : man{scopes, log}, log{log} {}

  void visit(ast::Func &func) override {
    const ScopeType oldScope = st.enterScope(ScopeType::func);
    auto funcSym = std::make_unique<sym::Func>();
    funcSym->loc = func.loc;
    if (func.ret) {
      funcSym->ret = {man.type(func.ret), sym::ValueCat::rvalue};
    } else {
      // @TODO infer return type
      log.warn(func.loc) << "Return type inference has not been implemented. "
        << "Return type is Void by default" << endlog;
      funcSym->ret = {man.lookup("Void", func.loc), sym::ValueCat::rvalue};
    }
    funcSym->params = man.funcParams(func.params);
    man.enterScope();
    for (size_t p = 0; p != func.params.size(); ++p) {
      auto paramSym = std::make_unique<sym::Object>();
      paramSym->loc = func.params[p].loc;
      paramSym->etype = funcSym->params[p];
      man.insert(func.params[p].name, std::move(paramSym));
    }
    for (const ast::StatPtr &stat : func.body.nodes) {
      stat->accept(*this);
    }
    man.leaveScope();
    man.insert(func.name, std::move(funcSym));
    st.leaveScope(oldScope);
  }
  sym::Symbol *objectType(
    const ast::TypePtr &type,
    const ast::ExprPtr &expr,
    const Loc loc
  ) {
    sym::Symbol *exprType;
    if (expr == nullptr) {
      exprType = nullptr;
    } else if (st.current() == ScopeType::func && st.parent() == ScopeType::struct_member) {
      exprType = exprMemFunc(man, expr.get(), parentStruct).type;
    } else if (st.current() == ScopeType::func && st.parent() == ScopeType::struct_static) {
      exprType = inferTypeStatFunc(man, expr.get(), parentStruct).type;
    } else {
      exprType = exprFunc(man, expr.get()).type;
    }
    sym::Symbol *symType = type ? man.type(type) : exprType;
    if (exprType != nullptr && exprType != symType) {
      log.ferror(loc) << "Expression and declaration type do not match" << endlog;
    }
    return symType;
  }
  void visit(ast::Var &var) override {
    auto varSym = std::make_unique<sym::Object>();
    varSym->loc = var.loc;
    varSym->etype.cat = sym::ValueCat::lvalue_var;
    varSym->etype.type = objectType(var.type, var.expr, var.loc);
    man.insert(var.name, std::move(varSym));
  }
  void visit(ast::Let &let) override {
    auto letSym = std::make_unique<sym::Object>();
    letSym->loc = let.loc;
    letSym->etype.cat = sym::ValueCat::lvalue_let;
    letSym->etype.type = objectType(let.type, let.expr, let.loc);
    man.insert(let.name, std::move(letSym));
  }
  void visit(ast::TypeAlias &alias) override {
    auto aliasSym = std::make_unique<sym::TypeAlias>();
    aliasSym->loc = alias.loc;
    aliasSym->type = man.type(alias.type);
    man.insert(alias.name, std::move(aliasSym));
  }
  
  void visit(ast::Init &) override {}
  void visit(ast::Struct &strut) override {
    auto structSym = std::make_unique<sym::StructType>();
    structSym->loc = strut.loc;
    structSym->scope = man.enterScope();
    for (const ast::Member &mem : strut.body) {
      // @TODO public, private
      if (mem.scope == ast::MemScope::member) {
        const ScopeType oldScope = st.enterScope(ScopeType::struct_member);
        mem.node->accept(*this);
        st.leaveScope(oldScope);
      } else {
        const ScopeType oldScope = st.enterScope(ScopeType::struct_static);
        mem.node->accept(*this);
        st.leaveScope(oldScope);
      }
    }
    man.leaveScope();
    man.insert(strut.name, std::move(structSym));
  }
  void visit(ast::Enum &num) override {
    auto enumSym = std::make_unique<sym::EnumType>();
    enumSym->loc = num.loc;
    enumSym->scope = man.enterScope();
    for (const ast::EnumCase &cs : num.cases) {
      auto caseSym = std::make_unique<sym::Object>();
      caseSym->loc = cs.loc;
      caseSym->etype.type = enumSym.get();
      caseSym->etype.cat = sym::ValueCat::lvalue_let;
      man.insert(cs.name, std::move(caseSym));
    }
    man.leaveScope();
    man.insert(num.name, std::move(enumSym));
  }

private:
  SymbolMan man;
  Log &log;
  sym::StructType *parentStruct = nullptr;
  ScopeTypes st;
};

}

void stela::traverse(sym::Scopes &scopes, const AST &ast, Log &log) {
  Visitor visitor{scopes, log};
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(visitor);
  }
}
