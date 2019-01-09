//
//  binding.hpp
//  STELA
//
//  Created by Indi Kernick on 13/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_binding_hpp
#define stela_binding_hpp

#include <tuple>
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

template <typename Elem>
Array<Elem> makeArray() {
  return make_retain<ArrayStorage<Elem>>();
}

template <typename, typename = void>
struct pass_traits;

/// Classes
template <typename T>
struct pass_traits<T, std::enable_if_t<std::is_aggregate_v<T>>> {
  using type = const T *;
  
  static type unwrap(const T &arg) noexcept {
    return &arg;
  }
  
  static constexpr bool nontrivial = true;
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

/// A wrapper around a compiled stela function. Acts as an ABI adapter to call
/// Stela functions using the Stela ABI
template <typename Fun, bool Member = false>
class Function;

template <bool Member, typename Ret, typename... Params>
class Function<Ret(Params...), Member> {
  template <typename Param, size_t Index, typename Tuple>
  static inline auto unwrap(const Tuple &params) noexcept {
    return pass_traits<Param>::unwrap(std::get<Index>(params));
  }
  
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;

  template <typename Tuple, size_t... Indicies>
  inline Ret call(const Tuple &params, std::index_sequence<Indicies...>) noexcept {
    if constexpr (param_ret) {
      std::aligned_storage_t<sizeof(Ret), alignof(Ret)> retStorage;
      Ret *retPtr = reinterpret_cast<Ret *>(&retStorage);
      if constexpr (Member) {
        ptr(unwrap<Params, Indicies>(params)..., retPtr);
      } else {
        ptr(nullptr, unwrap<Params, Indicies>(params)..., retPtr);
      }
      Ret retObj{std::move(*retPtr)};
      retPtr->~Ret();
      return retObj;
    } else if constexpr (std::is_void_v<Ret>) {
      if constexpr (Member) {
        ptr(unwrap<Params, Indicies>(params)...);
      } else {
        ptr(nullptr, unwrap<Params, Indicies>(params)...);
      }
    } else {
      if constexpr (Member) {
        return pass_traits<Ret>::wrap(ptr(unwrap<Params, Indicies>(params)...));
      } else {
        return pass_traits<Ret>::wrap(ptr(nullptr, unwrap<Params, Indicies>(params)...));
      }
    }
  }

public:
  static constexpr bool param_ret = pass_traits<Ret>::nontrivial;

  using type = std::conditional_t<
    Member,
    std::conditional_t<
      param_ret,
      void(pass_type<Params>..., Ret *) noexcept,
      pass_type<Ret>(pass_type<Params>...) noexcept
    >,
    std::conditional_t<
      param_ret,
      void(void *, pass_type<Params>..., Ret *) noexcept,
      pass_type<Ret>(void *, pass_type<Params>...) noexcept
    >
  >;
  
  explicit Function(const uint64_t addr) noexcept
    : ptr{reinterpret_cast<type *>(addr)} {}
  
  template <typename... Args>
  inline Ret operator()(Args &&... args) noexcept {
    std::tuple<Params...> params{std::forward<Args>(args)...};
    using Indicies = std::index_sequence_for<Params...>;
    return call(params, Indicies{});
  }
  
private:
  type *ptr;
};

}

#endif
