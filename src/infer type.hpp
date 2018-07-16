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

struct ExprType {
  sym::Symbol *type = nullptr;
  bool mut = false;
};

ExprType inferType(SymbolMan &, ast::Expression *);

}

#endif
