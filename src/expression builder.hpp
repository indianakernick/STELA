//
//  expression builder.hpp
//  STELA
//
//  Created by Indi Kernick on 16/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_expression_builder_hpp
#define stela_expression_builder_hpp

#include "ast.hpp"
#include "gen context.hpp"
#include "generate expr.hpp"
#include "function builder.hpp"

namespace stela {

class ExprBuilder {
public:
  ExprBuilder(gen::Ctx, FuncBuilder &);
  
  /// Get the value of an expression
  gen::Expr value(Scope &, ast::Expression *);
  /// Evaluate an expression
  void expr(Scope &, ast::Expression *, llvm::Value *);
  /// Evaluate an expression
  gen::Expr expr(Scope &, ast::Expression *);
  
  /// Create a conditional branch instruction
  void condBr(Scope &, ast::Expression *, llvm::BasicBlock *, llvm::BasicBlock *);

private:
  gen::Ctx ctx;
  FuncBuilder &fn;
};

}

#endif
