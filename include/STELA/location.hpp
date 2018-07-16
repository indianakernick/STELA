//
//  location.hpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_location_hpp
#define stela_location_hpp

#include <cstdint>

namespace stela {

using Line = uint32_t;
using Col = uint32_t;

/// A location in the source
struct Loc {
  Line l = 0;
  Col c = 0;
};

}

#endif
