//
//  binding.hpp
//  STELA
//
//  Created by Indi Kernick on 13/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_binding_hpp
#define stela_binding_hpp

#include "number.hpp"

namespace stela {

template <typename Fun>
class Function;

template <typename Ret, typename... Params>
class Function<Ret(Params...)> {
public:
  // @TODO find a cross platform way of setting the calling convention
  // There's a chance that the calling conventions of the C++ compiler and LLVM
  // will not match
  using Type = /* __cdecl */ Ret(void *, Params...);
  
  explicit Function(const uint64_t addr)
    : ptr{reinterpret_cast<Type *>(addr)} {}
  
  template <typename... Args>
  Ret operator()(Args... args) {
    return ptr(nullptr, args...);
  }
  
private:
  Type *ptr;
};

}

#endif
