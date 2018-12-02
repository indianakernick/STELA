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
#include "gen context.hpp"

namespace stela {

void generateDecl(gen::Ctx, const ast::Decls &);
gen::String generateDecl(gen::Ctx, const ast::Block &);

}

#endif
