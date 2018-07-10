//
//  lookup.hpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_lookup_hpp
#define stela_lookup_hpp

#include "symbols.hpp"

namespace stela {

sym::Func *lookup(const sym::Table &, sym::Name, const sym::FuncParams &);

}

#endif
