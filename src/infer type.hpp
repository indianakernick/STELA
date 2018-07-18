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
#include "symbol manager.hpp"

namespace stela {

sym::ExprType inferTypeFunc(SymbolMan &, ast::Expression *);
sym::ExprType inferTypeMemFunc(SymbolMan &, ast::Expression *, sym::StructType *);
sym::ExprType inferTypeStatFunc(SymbolMan &, ast::Expression *, sym::StructType *);

}

#endif
