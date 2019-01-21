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
#include <memory>
#include <algorithm>
#include "number.hpp"
#include <type_traits>
#include "retain ptr.hpp"

namespace stela {

template <typename Elem>
struct ArrayStorage : ref_count {
  ArrayStorage()
    : ref_count{} {}
  explicit ArrayStorage(const Uint len)
    : ref_count{}, cap{len}, len{len}, dat{alloc<Elem>(len)} {}
  ~ArrayStorage() {
    std::destroy_n(dat, len);
    dealloc(dat);
  }

  // uint64_t ref
  Uint cap = 0;
  Uint len = 0;
  Elem *dat = nullptr;
};

template <typename Elem>
using Array = retain_ptr<ArrayStorage<Elem>>;

template <typename Elem>
Array<Elem> makeEmptyArray() noexcept {
  return make_retain<ArrayStorage<Elem>>();
}

template <typename Elem>
Array<Elem> makeArray(const Uint size) noexcept {
  return make_retain<ArrayStorage<Elem>>(size);
}

template <typename Elem, typename... Args>
Array<Elem> makeArrayOf(Args &&... args) {
  Array<Elem> array = makeArray<Elem>(sizeof...(Args));
  Elem elems[] = {std::forward<Args>(args)...};
  std::uninitialized_move_n(elems, sizeof...(Args), array->dat);
  return array;
}

template <size_t Size>
Array<Char> makeString(const char (&value)[Size]) noexcept {
  constexpr size_t strSize = Size - 1;
  Array<Char> string = makeArray<Char>(strSize);
  std::copy_n(value, strSize, string->dat);
  string->len = strSize;
  return string;
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
  static T wrap(const type arg) noexcept {
    return *arg;
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

template <typename Fun>
struct Closure;

template <typename Fun>
struct pass_traits<Closure<Fun>> {
  using type = const Closure<Fun> *;
  
  static type unwrap(const Closure<Fun> &arg) noexcept {
    return &arg;
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
  explicit Function(type *const ptr) noexcept
    : ptr{ptr} {}
  
  template <typename... Args>
  inline Ret operator()(Args &&... args) noexcept {
    std::tuple<Params...> params{std::forward<Args>(args)...};
    using Indicies = std::index_sequence_for<Params...>;
    return call(params, Indicies{});
  }
  
private:
  type *ptr;
};

struct ClosureData : ref_count {
  ~ClosureData() {
    dtor(this);
  }
  
  // uint64_t ref
  void (*dtor)(void *);
};

template <typename Ret, typename... Params>
struct Closure<Ret(Params...)> {
  using Func = Function<Ret(ClosureData *, Params...), true>;
  
  template <typename... Args>
  inline auto operator()(Args &&... args) noexcept {
    return Func{fun}(dat.get(), std::forward<Args>(args)...);
  }
  
  Closure() = delete;
  explicit Closure(typename Func::type *fun) noexcept
    : fun{fun}, dat{nullptr} {}
  
  typename Func::type *fun;
  retain_ptr<ClosureData> dat;
};

namespace detail {

template <typename Fun>
struct is_noexcept;

template <typename Ret, typename... Params, bool Noexcept>
struct is_noexcept<Ret(Params...) noexcept(Noexcept)> {
  static constexpr bool value = Noexcept;
  using type = Ret(Params...);
};

template <auto FunPtr, typename Ret, typename... Params>
struct ClosureFunWrapParamRet {
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;
  
  static void call(ClosureData *, pass_type<Params>... params, Ret *ret) noexcept {
    new (ret) Ret{FunPtr(pass_traits<Ret>::wrap(params)...)};
  }
};

template <auto FunPtr, typename Ret, typename... Params>
struct ClosureFunWrapNormalRet {
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;
  
  static pass_type<Ret> call(ClosureData *, pass_type<Params>... params) noexcept {
    if constexpr (std::is_void_v<Ret>) {
      FunPtr(pass_traits<Ret>::wrap(params)...);
    } else {
      return pass_traits<Ret>::unwrap(
        FunPtr(pass_traits<Params>::wrap(params)...)
      );
    }
  }
};

template <typename Sig, auto FunPtr>
struct ClosureFunWrap;

template <auto FunPtr, typename Ret, typename... Params>
struct ClosureFunWrap<Ret(Params...), FunPtr> {
  using type = std::conditional_t<
    pass_traits<Ret>::nontrivial,
    ClosureFunWrapParamRet<FunPtr, Ret, Params...>,
    ClosureFunWrapNormalRet<FunPtr, Ret, Params...>
  >;
};

}

template <auto FunPtr>
auto makeClosureFromFunc() noexcept {
  using Sig = std::remove_pointer_t<decltype(FunPtr)>;
  static_assert(detail::is_noexcept<Sig>::value, "Stela cannot handle exceptions");
  using SigWithoutNoexcept = typename detail::is_noexcept<Sig>::type;
  return Closure<SigWithoutNoexcept>{
    detail::ClosureFunWrap<SigWithoutNoexcept, FunPtr>::type::call
  };
}

}

#endif
