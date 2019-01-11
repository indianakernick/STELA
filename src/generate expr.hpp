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
#include "categories.hpp"
#include "gen context.hpp"
#include "function builder.hpp"

namespace stela {

struct Object {
  llvm::Value *addr;
  ast::Type *type;
};
using Scope = std::vector<Object>;

gen::Expr generateValueExpr(Scope &, gen::Ctx, FuncBuilder &, ast::Expression *);
gen::Expr generateExpr(Scope &, gen::Ctx, FuncBuilder &, ast::Expression *, llvm::Value *);

}

#endif
