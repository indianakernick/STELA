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
#include "function builder.hpp"

namespace stela {

class ExprBuilder {
public:
  ExprBuilder(gen::Ctx, FuncBuilder &);
  
  /// Generate a zero value for a type
  llvm::Value *zero(ast::Type *);
  /// Get the address of an expression
  llvm::Value *addr(ast::Expression *);
  /// Get the value of an expression
  llvm::Value *value(ast::Expression *);
  /// Evaluate a discarded expression
  void discard(ast::Expression *);
  
  /// Create a conditional branch instruction
  void condBr(ast::Expression *, llvm::BasicBlock *, llvm::BasicBlock *);

private:
  gen::Ctx ctx;
  FuncBuilder &fn;
};

}

#endif
