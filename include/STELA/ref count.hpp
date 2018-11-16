//
//  ref count.hpp
//  STELA
//
//  Created by Indi Kernick on 17/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_ref_count_hpp
#define stela_ref_count_hpp

namespace stela {

template <typename T>
struct retain_traits;

template <typename T>
class ref_count {
  friend retain_traits<T>;
  unsigned count = 1;
  
protected:
  ref_count() = default;
};

}

#endif
