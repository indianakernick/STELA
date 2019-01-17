//
//  generate expr.hpp
//  STELA
//
//  Created by Indi Kernick on 1/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_expr_hpp
#define stela_generate_expr_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "categories.hpp"
#include "gen context.hpp"
#include "function builder.hpp"

namespace stela {

struct Object {
  llvm::Value *addr;
  ast::Type *type;
};
using Scope = std::vector<Object>;

namespace gen {

struct Func {
  FuncBuilder &builder;
  llvm::Value *closure;
  // sym::Func or sym::Lambda
  sym::Symbol *symbol;
};

}

gen::Expr generateValueExpr(Scope &, gen::Ctx, gen::Func, ast::Expression *);
gen::Expr generateBoolExpr(Scope &, gen::Ctx, gen::Func, ast::Expression *);
gen::Expr generateExpr(Scope &, gen::Ctx, gen::Func, ast::Expression *, llvm::Value *);

}

#endif
