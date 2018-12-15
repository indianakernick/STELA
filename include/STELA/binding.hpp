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
#include <type_traits>

namespace stela {

/// A wrapper around a compiled stela function. Acts as an ABI adapter to call
/// Stela functions using the Stela ABI
template <typename Fun>
class Function;

template <typename Ret, typename... Params>
class Function<Ret(Params...)> {
  template <typename Type>
  using ConvertParam = std::conditional_t<
    std::is_class_v<Type>,
    const Type *,
    Type
  >;
  
  template <typename Arg>
  static auto convertArg(Arg &arg) noexcept {
    if constexpr (std::is_class_v<Arg>) {
      static_assert(
        std::is_trivially_copyable_v<Arg>,
        "Only trivially copyable classes can be passed-by-value"
      );
      return &arg;
    } else {
      return arg;
    }
  }

public:
  using Type = std::conditional_t<
    std::is_class_v<Ret>,
    void(void *, ConvertParam<Params>..., Ret *) noexcept,
    Ret(void *, ConvertParam<Params>...) noexcept
  >;
  
  explicit Function(const uint64_t addr)
    : ptr{reinterpret_cast<Type *>(addr)} {}
  
  template <typename... Args>
  inline Ret operator()(const Args &... args) noexcept {
    if constexpr (std::is_class_v<Ret>) {
      Ret ret;
      ptr(nullptr, convertArg(args)..., &ret);
      return ret;
    } else {
      return ptr(nullptr, convertArg(args)...);
    }
  }
  
private:
  Type *ptr;
};

}

#endif
