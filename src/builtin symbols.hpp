//
//  builtin symbols.hpp
//  STELA
//
//  Created by Indi Kernick on 16/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_builtin_symbols_hpp
#define stela_builtin_symbols_hpp

#include "ast.hpp"
#include "symbols.hpp"

namespace stela {

bool validIncr(ast::BuiltinType *);
bool validOp(ast::UnOp, ast::BuiltinType *);
ast::BuiltinType *validOp(const sym::Builtins &, ast::BinOp, ast::BuiltinType *, ast::BuiltinType *);
bool validOp(ast::AssignOp, ast::BuiltinType *, ast::BuiltinType *);

sym::Module makeBuiltinModule(sym::Builtins &);

}

#endif
