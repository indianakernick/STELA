//
//  expr stack.hpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef engine_expr_stack_hpp
#define engine_expr_stack_hpp

#include "symbols.hpp"

namespace stela {

enum class ExprKind {
  call,
  member,
  free_fun,
  expr,
  subexpr
};

class ExprStack {
public:
  void pushCall();
  void pushExpr(const sym::ExprType &);
  void pushMember(const sym::Name &);
  void pushMemberExpr(const ast::TypePtr &);
  void pushFunc(const sym::Name &, sym::Scope *);
  
  ExprKind top() const;
  /// top == kind && below top == member && below below top != call
  bool memVarExpr(ExprKind) const;
  /// top == kind && below top == member && below below top == call
  bool memFunExpr(ExprKind) const;
  /// top == kind && below top == call
  bool call(ExprKind) const;
  
  struct NameScope {
    sym::Name name;
    sym::Scope *scope;
  };
  void popCall();
  sym::Name popMember();
  sym::ExprType popExpr();
  NameScope popFunc();
  
  void setExpr(sym::ExprType);
  void enterSubExpr();
  sym::ExprType leaveSubExpr();
  
private:
  struct Expr {
    ExprKind kind;
    sym::Name name;
    sym::Scope *scope;
    
    explicit Expr(ExprKind);
    Expr(ExprKind, const sym::Name &);
    Expr(ExprKind, const sym::Name &, sym::Scope *);
  };
  
  std::vector<Expr> exprs;
  sym::ExprType exprType = sym::null_type;
  
  void pop(ExprKind);
};

}

#endif
