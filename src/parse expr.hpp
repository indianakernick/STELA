//
//  parse expr.hpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef parse_expr_hpp
#define parse_expr_hpp

#include "ast.hpp"
#include "parse tokens.hpp"

namespace stela {

ast::ExprPtr parseExpr(ParseTokens &);

}

#endif
