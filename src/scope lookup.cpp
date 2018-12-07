//
//  scope lookup.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope lookup.hpp"

#include "symbol desc.hpp"
#include "compare types.hpp"
#include "scope traverse.hpp"

using namespace stela;

namespace {

ast::TypeAlias *lookupTypeImpl(Log &log, sym::Scope *scope, ast::NamedType &type) {
  if (sym::Symbol *symbol = find(scope, sym::Name{type.name})) {
    if (auto *alias = dynamic_cast<sym::TypeAlias *>(symbol)) {
      type.definition = alias->node.get();
      alias->referenced = true;
      return type.definition;
    }
    log.error(type.loc) << "The name \"" << type.name << "\" does not refer to a type" << fatal;
  }
  if (sym::Scope *parent = scope->parent) {
    return lookupTypeImpl(log, parent, type);
  } else {
    log.error(type.loc) << "Expected type name but found \"" << type.name << "\"" << fatal;
  }
}

}

sym::Symbol *stela::find(sym::Scope *scope, const sym::Name &name) {
  const auto iter = scope->table.find(name);
  if (iter == scope->table.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

SymbolScope stela::findScope(sym::Scope *scope, const sym::Name &name) {
  if (sym::Symbol *symbol = find(scope, name)) {
    return {symbol, scope};
  }
  if (sym::Scope *parent = scope->parent) {
    return findScope(parent, name);
  } else {
    return {nullptr, nullptr};
  }
}

ast::Name stela::moduleName(sym::Scope *scope) {
  sym::Scope *const global = findNearest(sym::ScopeType::ns, scope);
  assert(global);
  return global->module;
}

ast::TypeAlias *stela::lookupTypeName(sym::Ctx ctx, ast::NamedType &type) {
  if (type.definition) {
    return type.definition;
  } else {
    return lookupTypeImpl(ctx.log, ctx.man.cur(), type);
  }
}

ast::TypePtr stela::lookupStrongType(sym::Ctx ctx, const ast::TypePtr &type) {
  if (auto named = dynamic_pointer_cast<ast::NamedType>(type)) {
    ast::TypeAlias *alias = lookupTypeName(ctx, *named);
    if (alias->strong) {
      return type;
    } else {
      return lookupStrongType(ctx, alias->type);
    }
  } else {
    return type;
  }
}

ast::TypePtr stela::lookupConcreteType(sym::Ctx ctx, const ast::TypePtr &type) {
  if (auto named = dynamic_pointer_cast<ast::NamedType>(type)) {
    ast::TypeAlias *alias = lookupTypeName(ctx, *named);
    return lookupConcreteType(ctx, alias->type);
  } else {
    return type;
  }
}

namespace {

template <typename Func>
void writeFuncType(ast::FuncType &funcType, const Func &func) {
  funcType.params.reserve(func.params.size());
  for (const ast::FuncParam &param : func.params) {
    funcType.params.push_back(ast::ParamType{param.ref, param.type});
  }
  funcType.ret = func.ret;
}

}

ast::TypePtr stela::getFuncType(Log &log, ast::Func &func, Loc loc) {
  if (func.receiver) {
    log.error(loc) << "Cannot take address of member function" << fatal;
  }
  auto funcType = make_retain<ast::FuncType>();
  writeFuncType(*funcType, func);
  return funcType;
}

ast::TypePtr stela::getLambdaType(sym::Ctx ctx, ast::Lambda &lam) {
  if (lam.ret) {
    validateType(ctx, lam.ret);
  }
  for (const ast::FuncParam &param : lam.params) {
    validateType(ctx, param.type);
  }
  auto funcType = make_retain<ast::FuncType>();
  writeFuncType(*funcType, lam);
  return funcType;
}

void stela::checkIdentShadow(sym::Ctx ctx, const sym::Name &name, const Loc loc) {
  const auto [symbol, scope] = findScope(ctx.man.cur()->parent, name);
  if (symbol) {
    ctx.log.warn(loc) << "Declaration of \"" << name << "\" shadows a "
      << symbolDesc(symbol) << " at " << moduleName(scope) << ':' << symbol->loc << endlog;
  }
}
