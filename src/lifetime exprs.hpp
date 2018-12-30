//
//  lifetime exprs.hpp
//  STELA
//
//  Created by Indi Kernick on 25/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_lifetime_exprs_hpp
#define stela_lifetime_exprs_hpp

#include <llvm/IR/IRBuilder.h>

namespace stela {

namespace ast {

struct Type;

}

namespace gen {

class FuncInst;

}

class LifetimeExpr {
public:
  LifetimeExpr(gen::FuncInst &, llvm::IRBuilder<> &);

  void defConstruct(ast::Type *, llvm::Value *);
  void copyConstruct(ast::Type *, llvm::Value *, llvm::Value *);
  void moveConstruct(ast::Type *, llvm::Value *, llvm::Value *);
  void copyAssign(ast::Type *, llvm::Value *, llvm::Value *);
  void moveAssign(ast::Type *, llvm::Value *, llvm::Value *);
  void relocate(ast::Type *, llvm::Value *, llvm::Value *);
  void destroy(ast::Type *, llvm::Value *);

private:
  gen::FuncInst &inst;
  llvm::IRBuilder<> &ir;
};

}

#endif
