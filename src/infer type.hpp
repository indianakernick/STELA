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
#include "context.hpp"

namespace stela {

sym::ExprType getExprType(sym::Ctx, const ast::ExprPtr &, const ast::TypePtr &);

}

#endif
