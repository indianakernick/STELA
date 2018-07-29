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

/// Visitor for ast::MemScope::member. Node is a declaration
class StructMemberVisitor final : ast::Visitor {
public:
  StructMemberVisitor();

  void visit(ast::Func &) {}
  void visit(ast::Var &) {}
  void visit(ast::Let &) {}
  // implicitly static
  void visit(ast::TypeAlias &) {}
  void visit(ast::Init &) {}
  // implicitly static
  void visit(ast::Struct &) {}
  // implicitly static
  void visit(ast::Enum &) {}

private:
  
};

enum class ScopeType {
  static_mem,
  inst_mem,
  global
};

class Visitor final : public ast::Visitor {
public:
  Visitor(sym::Scopes &scopes, Log &log)
    : man{scopes, log}, log{log} {}

  void visit(ast::Func &func) override {
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
    man.enterScope(sym::ScopeType::function);
    for (size_t p = 0; p != func.params.size(); ++p) {
      auto paramSym = std::make_unique<sym::Object>();
      paramSym->loc = func.params[p].loc;
      paramSym->etype = funcSym->params[p];
      man.insert(std::string(func.params[p].name), std::move(paramSym));
    }
    if (scopeType == ScopeType::inst_mem) {
      auto selfSym = std::make_unique<sym::Object>();
      selfSym->loc = funcSym->loc;
      selfSym->etype = sym::ExprType{parentStruct, sym::ValueCat::lvalue_var};
      funcSym->params.insert(funcSym->params.begin(), selfSym->etype);
      man.insert(std::string("self"), std::move(selfSym));
    }
    for (const ast::StatPtr &stat : func.body.nodes) {
      stat->accept(*this);
    }
    man.leaveScope();
    man.insert(parentScopeName + std::string(func.name), std::move(funcSym));
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
    auto varSym = std::make_unique<sym::Object>();
    varSym->loc = var.loc;
    varSym->etype.cat = sym::ValueCat::lvalue_var;
    varSym->etype.type = objectType(var.type, var.expr, var.loc);
    if (scopeType == ScopeType::static_mem) {
      man.insert(parentScopeName + std::string(var.name), std::move(varSym));
    } else {
      man.insert(std::string(var.name), std::move(varSym));
    }
  }
  void visit(ast::Let &let) override {
    auto letSym = std::make_unique<sym::Object>();
    letSym->loc = let.loc;
    letSym->etype.cat = sym::ValueCat::lvalue_let;
    letSym->etype.type = objectType(let.type, let.expr, let.loc);
    if (scopeType == ScopeType::static_mem) {
      man.insert(parentScopeName + std::string(let.name), std::move(letSym));
    } else {
      man.insert(std::string(let.name), std::move(letSym));
    }
  }
  void visit(ast::TypeAlias &alias) override {
    auto aliasSym = std::make_unique<sym::TypeAlias>();
    aliasSym->loc = alias.loc;
    aliasSym->type = man.type(alias.type);
    man.insert(parentScopeName + std::string(alias.name), std::move(aliasSym));
  }
  
  void visit(ast::Init &) override {}
  void visit(ast::Struct &strut) override {
    auto structSym = std::make_unique<sym::StructType>();
    structSym->loc = strut.loc;
    structSym->scope = man.enterScope(sym::ScopeType::structure);
    std::vector<bool> instanceData(strut.body.size(), false);
    const ScopeType oldScopeType = std::exchange(scopeType, ScopeType::inst_mem);
    for (size_t m = 0; m != strut.body.size(); ++m) {
      const ast::Member &mem = strut.body[m];
      if (mem.scope == ast::MemScope::member) {
        if (auto *var = dynamic_cast<ast::Var *>(mem.node.get())) {
          instanceData[m] = true;
          visit(*var);
          continue;
        } else if (auto *let = dynamic_cast<ast::Let *>(mem.node.get())) {
          instanceData[m] = true;
          visit(*let);
          continue;
        }
      }
    }
    man.leaveScope();
    const size_t oldScope = pushScopeName(strut.name);
    sym::StructType *const oldStruct = std::exchange(parentStruct, structSym.get());
    for (size_t m = 0; m != strut.body.size(); ++m) {
      const ast::Member &mem = strut.body[m];
      if (!instanceData[m]) {
        if (mem.scope == ast::MemScope::member) {
          scopeType = ScopeType::inst_mem;
        } else {
          scopeType = ScopeType::static_mem;
        }
        mem.node->accept(*this);
      }
    }
    parentStruct = oldStruct;
    popScopeName(oldScope);
    scopeType = oldScopeType;
    man.insert(parentScopeName + std::string(strut.name), std::move(structSym));
  }
  void visit(ast::Enum &num) override {
    auto enumSym = std::make_unique<sym::EnumType>();
    enumSym->loc = num.loc;
    man.insert(parentScopeName + std::string(num.name), std::move(enumSym));
    const size_t oldScope = pushScopeName(num.name);
    for (const ast::EnumCase &cs : num.cases) {
      auto caseSym = std::make_unique<sym::Object>();
      caseSym->loc = cs.loc;
      caseSym->etype.type = enumSym.get();
      caseSym->etype.cat = sym::ValueCat::lvalue_let;
      man.insert(parentScopeName + std::string(cs.name), std::move(caseSym));
    }
    popScopeName(oldScope);
  }

private:
  SymbolMan man;
  Log &log;
  sym::Scope *parentScope = nullptr;
  std::string parentScopeName = {};
  ScopeType scopeType = ScopeType::global;
  sym::StructType *parentStruct = nullptr;
  
  size_t pushScopeName(const std::string_view name) {
    const size_t oldScopeSize = parentScopeName.size();
    parentScopeName += '_';
    parentScopeName.append(name);
    parentScopeName += '_';
    return oldScopeSize;
  }
  void popScopeName(const size_t oldScopeSize) {
    parentScopeName.erase(oldScopeSize);
  }
};

}

void stela::traverse(sym::Scopes &scopes, const AST &ast, Log &log) {
  Visitor visitor{scopes, log};
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(visitor);
  }
}
