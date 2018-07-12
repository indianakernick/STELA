//
//  type name.hpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_type_name_hpp
#define stela_type_name_hpp

#include <string>
#include "ast.hpp"
#include "symbols.hpp"

namespace stela {

std::string typeName(const sym::Scope &, const ast::TypePtr &);
std::string funcName(const sym::Scope &, std::string_view, const ast::FuncParams &);
sym::FuncParams funcParams(const sym::Scope &, const ast::FuncParams &);

}

#endif
