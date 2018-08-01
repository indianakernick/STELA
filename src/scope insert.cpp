//
//  scope insert.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope insert.hpp"

#include "scope find.hpp"
#include "compare params args.hpp"

using namespace stela;

namespace {

class InsertVisitor final : public sym::ScopeVisitor {
public:
  InsertVisitor(const sym::Name &name, sym::SymbolPtr symbol)
    : name{name}, symbol{std::move(symbol)} {}

  void visit(sym::NSScope &scope) {
    insert(scope.table);
  }
  void visit(sym::FuncScope &scope) {
    insert(scope.table);
  }
  void visit(sym::StructScope &) {
    assert(false);
  }
  void visit(sym::EnumScope &scope) {
    scope.table.push_back({name, std::move(symbol)});
  }

private:
  const sym::Name &name;
  sym::SymbolPtr symbol;
  
  void insert(sym::UnorderedTable &table) {
    table.insert({name, std::move(symbol)});
  }
};

void insertSymbol(sym::Scope *const scope, const sym::Name &name, sym::SymbolPtr symbol) {
  InsertVisitor inserter{name, std::move(symbol)};
  scope->accept(inserter);
}

}

void stela::insert(
  sym::Scope *const scope,
  Log &log,
  const sym::Name name,
  sym::SymbolPtr symbol
) {
  sym::Symbol *const dupSym = find(scope, name);
  if (dupSym) {
    log.ferror(symbol->loc) << "Redefinition of symbol \"" << name
      << "\" previously declared at " << dupSym->loc << endlog;
  } else {
    insertSymbol(scope, name, std::move(symbol));
  }
}

sym::Func *stela::insert(
  sym::Scope *const scope,
  Log &log,
  const sym::Name name,
  sym::FuncPtr func
) {
  const std::vector<sym::Symbol *> symbols = findMany(scope, name);
  for (sym::Symbol *const symbol : symbols) {
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(symbol);
    if (dupFunc) {
      if (sameParams(dupFunc->params, func->params)) {
        log.ferror(func->loc) << "Redefinition of function \"" << name
          << "\" previously declared at " << symbol->loc << endlog;
      }
    } else {
      log.ferror(func->loc) << "Redefinition of function \"" << name
        << "\" previously declared (as a different kind of symbol) at "
        << symbol->loc << endlog;
    }
  }
  sym::Func *const ret = func.get();
  insertSymbol(scope, name, std::move(func));
  return ret;
}

void stela::insert(
  sym::StructScope *const scope,
  Log &log,
  const sym::MemKey &key,
  sym::SymbolPtr symbol
) {
  for (const sym::StructTableRow &row : scope->table) {
    if (row.name == key.name) {
      log.ferror(symbol->loc) << "Redefinition of member \"" << key.name
        << "\" previously declared at " << row.val->loc << endlog;
    }
  }
  scope->table.push_back({key.name, key.access, key.scope, std::move(symbol)});
}

sym::Func *stela::insert(
  sym::StructScope *const scope,
  Log &log,
  const sym::MemKey &key,
  sym::FuncPtr func
) {
  for (const sym::StructTableRow &row : scope->table) {
    if (row.name != key.name) {
      continue;
    }
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(row.val.get());
    if (dupFunc) {
      if (row.scope != key.scope) {
        log.ferror(func->loc) << "Cannot overload static and non-static member functions \""
          << row.name << '"' << endlog;
      }
      if (sameParams(dupFunc->params, func->params)) {
        log.ferror(func->loc) << "Redefinition of function \"" << key.name
          << "\" previously declared at " << row.val->loc << endlog;
      }
    } else {
      log.ferror(func->loc) << "Redefinition of function \"" << key.name
        << "\" previously declared (as a different kind of symbol) at "
        << row.val->loc << endlog;
    }
  }
  sym::Func *const ret = func.get();
  scope->table.push_back({key.name, key.access, key.scope, std::move(func)});
  return ret;
}
