//
//  lifetime exprs.hpp
//  STELA
//
//  Created by Indi Kernick on 25/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef engine_lifetime_exprs_hpp
#define engine_lifetime_exprs_hpp

#include <llvm/IR/IRBuilder.h>

namespace stela {

namespace ast {

struct Type;

}

namespace gen {

class FuncInst;

}

// @TODO we'll need to use this to determine how to call and return
enum class TypeCat {
  /// Primitive types like integers and floats.
  /// Default constructor just sets to 0. Destructor does nothing.
  /// Other special functions just copy.
  trivially_copyable,
  /// Arrays and closures.
  /// Requires calls to special functions but can be relocated.
  /// Moving and then destroying is the same as copying.
  trivially_relocatable,
  /// Structs and some user types.
  /// Requires calls to special functions.
  /// Passed to functions by pointer and returned by pointer.
  nontrivial
};

TypeCat classifyType(ast::Type *);

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
