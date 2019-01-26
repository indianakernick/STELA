//
//  parse func.hpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_parse_func_hpp
#define stela_parse_func_hpp

#include "ast.hpp"
#include "parse tokens.hpp"

namespace stela {

ast::FuncParams parseFuncParams(ParseTokens &);
ast::TypePtr parseFuncRet(ParseTokens &);
ast::Block parseFuncBody(ParseTokens &);
ast::DeclPtr parseFunc(ParseTokens &, bool);

}

#endif
