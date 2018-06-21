//
//  syntax analysis.hpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_syntax_analysis_hpp
#define stela_syntax_analysis_hpp

#include "ast.hpp"
#include "token.hpp"

namespace stela {

AST createAST(const Tokens &);

}

#endif
