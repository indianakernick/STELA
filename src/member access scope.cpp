//
//  member access scope.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "member access scope.hpp"

using namespace stela;

namespace {

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

}

sym::MemAccess stela::memAccess(const ast::Member &mem, Log &) {
  // @TODO maybe make vars private by default and functions public by default?
  if (mem.access == ast::MemAccess::private_) {
    return sym::MemAccess::private_;
  } else {
    return sym::MemAccess::public_;
  }
}

sym::MemScope stela::memScope(const ast::Member &mem, Log &log) {
  MemberScope visitor{log, mem.scope};
  mem.node->accept(visitor);
  return visitor.scope;
}
