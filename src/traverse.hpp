//
//  traverse.hpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_traverse_hpp
#define stela_traverse_hpp

#include "context.hpp"

namespace stela {

void traverse(sym::Ctx, const sym::Module &);
void traverse(sym::Ctx, const ast::Block &);

}

#endif
