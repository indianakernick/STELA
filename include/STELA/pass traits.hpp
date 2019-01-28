//
//  pass traits.hpp
//  STELA
//
//  Created by Indi Kernick on 27/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_pass_traits_hpp
#define stela_pass_traits_hpp

#include "native types.hpp"

namespace stela {

template <typename, typename = void>
struct pass_traits;

/// Classes
template <typename T>
struct pass_traits<T, std::enable_if_t<std::is_aggregate_v<T>>> {
  using type = const T *;
  
  static type unwrap(const T &arg) noexcept {
    return &arg;
  }
  static T wrap(const type arg) noexcept {
    return *arg;
  }
  
  static constexpr bool nontrivial = true;
};

// @TODO deal with classes

/// Enums
template <typename T>
struct pass_traits<T, std::enable_if_t<std::is_enum_v<T>>> {
  using type = typename pass_traits<std::underlying_type_t<T>>::type;
  
  static type unwrap(const T arg) noexcept {
    return static_cast<type>(arg);
  }
  static T wrap(const type arg) noexcept {
    return static_cast<T>(arg);
  }
  
  static constexpr bool nontrivial = false;
};

/// References
template <typename T>
struct pass_traits<T &> {
  using type = T *;
  
  static type unwrap(T &arg) noexcept {
    return &arg;
  }
  static T &wrap(type arg) noexcept {
    return *arg;
  }
  
  static constexpr bool nontrivial = false;
};

/// Pointers
template <typename T>
struct pass_traits<T *> {
  using type = T *;
  
  static type unwrap(T *arg) noexcept {
    return arg;
  }
  static T *wrap(type arg) noexcept {
    return arg;
  }
  
  static constexpr bool nontrivial = false;
};

#define PASS_PRIMITIVE(TYPE)                                                    \
  template <>                                                                   \
  struct pass_traits<TYPE> {                                                    \
    using type = TYPE;                                                          \
    static type unwrap(TYPE arg) noexcept {                                     \
      return arg;                                                               \
    }                                                                           \
    static TYPE wrap(type arg) noexcept {                                       \
      return arg;                                                               \
    }                                                                           \
    static constexpr bool nontrivial = false;                                   \
  }

PASS_PRIMITIVE(Opaq);
PASS_PRIMITIVE(Bool);
PASS_PRIMITIVE(Byte);
PASS_PRIMITIVE(Char);
PASS_PRIMITIVE(Real);
PASS_PRIMITIVE(Sint);
PASS_PRIMITIVE(Uint);

#undef PASS_PRIMITIVE

template <>
struct pass_traits<void> {
  using type = void;
  static constexpr bool nontrivial = false;
};

template <typename Elem>
struct pass_traits<Array<Elem>> {
  using type = ArrayStorage<Elem> *const *;
  
  static type unwrap(const Array<Elem> &arg) noexcept {
    return reinterpret_cast<ArrayStorage<Elem> *const *>(&arg);
  }
  
  static constexpr bool nontrivial = true;
};

template <typename Fun>
struct pass_traits<Closure<Fun>> {
  using type = const Closure<Fun> *;
  
  static type unwrap(const Closure<Fun> &arg) noexcept {
    return &arg;
  }
  
  static constexpr bool nontrivial = true;
};

}

#endif
