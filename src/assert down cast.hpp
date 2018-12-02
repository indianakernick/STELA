//
//  assert down cast.hpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_assert_down_cast_hpp
#define stela_assert_down_cast_hpp

#include <cassert>

namespace stela {

#ifdef NDEBUG

template <typename Derived, typename Base>
Derived *assertDownCast(Base *const base) noexcept {
  return static_cast<Derived *>(base);
}

#else

template <typename Derived, typename Base>
Derived *assertDownCast(Base *const base) noexcept {
  Derived *const ptr = dynamic_cast<Derived *>(base);
  assert(ptr);
  return ptr;
}

#endif
  
}

#endif
