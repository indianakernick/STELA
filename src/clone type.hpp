//
//  clone type.hpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_clone_type_hpp
#define stela_clone_type_hpp

#include "ast.hpp"

namespace stela {

ast::TypePtr clone(const ast::TypePtr &);

}

#endif
