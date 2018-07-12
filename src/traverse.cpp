//
//  traverse.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "traverse.hpp"

#include "lookup.hpp"
#include "type name.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(sym::Scopes &scopes, Log &log)
    : scopes{scopes}, log{log}, current{scopes.back().get()} {}

  void visit(ast::Func &func) override {
    auto funcSym = std::make_unique<sym::Func>();
    funcSym->loc = func.loc;
    if (func.ret) {
      funcSym->ret = typeName(*current, func.ret, log);
    } else {
      // @TODO infer return type
      log.ferror(func.loc) << "Return type inference has not been implemented" << endlog;
      return;
    }
    funcSym->params = funcParams(*current, func.params, log);
    std::string name = funcName(*current, func.name, func.params, log);
    sym::Symbol *dup = lookupDup(current->table, name);
    if (!dup) {
      current->table.insert({name, std::move(funcSym)});
    } else {
      log.ferror(func.loc) << "Redefinition of function " << func.name
        << " previously declared at " << dup->loc << endlog;
    }
    
    sym::ScopePtr funcScope = std::make_unique<sym::Scope>();
    funcScope->parent = current;
    current = funcScope.get();
    // process parameters and body
    current = funcScope->parent;
    scopes.push_back(std::move(funcScope));
  }
  
  void visit(ast::Var &) override {}
  void visit(ast::Let &) override {}
  void visit(ast::TypeAlias &alias) override {
    auto aliasSym = std::make_unique<sym::TypeAlias>();
    aliasSym->loc = alias.loc;
    aliasSym->type = typeName(*current, alias.type, log);
    current->table.insert({std::string(alias.name), std::move(aliasSym)});
  }
  void visit(ast::Init &) override {}
  void visit(ast::Struct &) override {}
  void visit(ast::Enum &) override {}

private:
  sym::Scopes &scopes;
  Log &log;
  sym::Scope *current;
};

}

void stela::traverse(sym::Scopes &scopes, const AST &ast, Log &log) {
  Visitor visitor{scopes, log};
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(visitor);
  }
}
