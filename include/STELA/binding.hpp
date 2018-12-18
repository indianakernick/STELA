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
#include "retain ptr.hpp"

namespace stela {

template <typename Elem>
struct ArrayStorage : ref_count {
  ArrayStorage()
    : ref_count{} {}

  // uint64_t ref
  Uint cap = 0;
  Uint len = 0;
  Elem *dat = nullptr;
};

template <typename Elem>
using Array = retain_ptr<ArrayStorage<Elem>>;

template <typename T>
struct pass_traits {
  using type = std::conditional_t<
    std::is_class_v<T>,
    const T *,
    T
  >;
  
  static type unwrap(const T &arg) noexcept {
    if constexpr (std::is_class_v<T>) {
      static_assert(
        std::is_trivially_copyable_v<T>,
        "Only trivially copyable classes can be passed-by-value"
      );
      return &arg;
    } else {
      return arg;
    }
  }
  static T wrap(type arg) noexcept {
    return arg;
  }
  
  static constexpr bool param_ret = std::is_class_v<T>;
};

template <>
struct pass_traits<void> {
  using type = void;
  static constexpr bool param_ret = false;
};

template <typename Elem>
struct pass_traits<retain_ptr<ArrayStorage<Elem>>> {
  using type = ArrayStorage<Elem> *;
  
  static type unwrap(const Array<Elem> &arg) noexcept {
    Array<Elem> copy = arg;
    return copy.detach();
  }
  static type unwrap(Array<Elem> &&arg) noexcept {
    return arg.detach();
  }
  static Array<Elem> wrap(type arg) noexcept {
    return retain_ptr{arg};
  }
  
  static constexpr bool param_ret = false;
};

/// A wrapper around a compiled stela function. Acts as an ABI adapter to call
/// Stela functions using the Stela ABI
template <typename Fun>
class Function;

template <typename Ret, typename... Params>
class Function<Ret(Params...)> {
  template <typename Arg>
  static inline auto unwrap(Arg &&arg) noexcept {
    return pass_traits<std::decay_t<Arg>>::unwrap(std::forward<Arg>(arg));
  }
  
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;

public:
  static constexpr bool param_ret = pass_traits<Ret>::param_ret;

  using type = std::conditional_t<
    param_ret,
    void(void *, pass_type<Params>..., Ret *) noexcept,
    pass_type<Ret>(void *, pass_type<Params>...) noexcept
  >;
  
  explicit Function(const uint64_t addr)
    : ptr{reinterpret_cast<type *>(addr)} {}
  
  template <typename... Args>
  inline Ret operator()(Args &&... args) noexcept {
    if constexpr (param_ret) {
      Ret ret;
      ptr(nullptr, unwrap(std::forward<Args>(args))..., &ret);
      return ret;
    } else if constexpr (std::is_void_v<Ret>) {
      ptr(nullptr, unwrap(std::forward<Args>(args))...);
    } else {
      return pass_traits<Ret>::wrap(ptr(nullptr, unwrap(std::forward<Args>(args))...));
    }
  }
  
private:
  type *ptr;
};

}

#endif
