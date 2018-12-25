//
//  lifetime exprs.hpp
//  STELA
//
//  Created by Indi Kernick on 25/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef engine_lifetime_exprs_hpp
#define engine_lifetime_exprs_hpp

#include "gen context.hpp"
#include <llvm/IR/IRBuilder.h>

namespace stela {

namespace ast {

struct Type;

}

class LifetimeExpr {
public:
  LifetimeExpr(gen::Ctx, llvm::IRBuilder<> &);

  void defConstruct(ast::Type *, llvm::Value *);
  void copyConstruct(ast::Type *, llvm::Value *, llvm::Value *);
  void moveConstruct(ast::Type *, llvm::Value *, llvm::Value *);
  void copyAssign(ast::Type *, llvm::Value *, llvm::Value *);
  void moveAssign(ast::Type *, llvm::Value *, llvm::Value *);
  void relocate(ast::Type *, llvm::Value *, llvm::Value *);
  void destroy(ast::Type *, llvm::Value *);

private:
  gen::Ctx ctx;
  llvm::IRBuilder<> &ir;
};

}

#endif
