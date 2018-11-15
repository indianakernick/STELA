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
#include "builtin symbols.hpp"

namespace stela {

sym::ExprType getExprType(Symbols &, ScopeMan &, Log &, ast::Expression *);

}

#endif
