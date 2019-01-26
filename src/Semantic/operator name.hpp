//
//  operator name.hpp
//  STELA
//
//  Created by Indi Kernick on 16/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_operator_name_hpp
#define stela_operator_name_hpp

#include "ast.hpp"

namespace stela {

std::string_view opName(ast::AssignOp);
std::string_view opName(ast::BinOp);
std::string_view opName(ast::UnOp);
std::string_view typeName(ast::BtnTypeEnum);

}

#endif
