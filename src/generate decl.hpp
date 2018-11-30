//
//  generate decl.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_decl_hpp
#define stela_generate_decl_hpp

#include "ast.hpp"
#include "log output.hpp"

namespace stela {

void generateDecl(std::string &, Log &, const ast::Decls &);

}

#endif
