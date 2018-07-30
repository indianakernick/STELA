//
//  infer type.hpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_infer_type_hpp
#define stela_infer_type_hpp

#include "ast.hpp"
#include "log output.hpp"
#include "scope manager.hpp"

namespace stela {

sym::ExprType exprFunc(ScopeMan &, Log &, ast::Expression *);
sym::ExprType exprMemFunc(ScopeMan &, Log &, ast::Expression *, sym::StructType *);
sym::ExprType exprStatFunc(ScopeMan &, Log &, ast::Expression *, sym::StructType *);

}

#endif
