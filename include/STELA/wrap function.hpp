//
//  wrap function.hpp
//  STELA
//
//  Created by Indi Kernick on 27/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_wrap_function_hpp
#define stela_wrap_function_hpp

#include "pass traits.hpp"

namespace stela {

namespace detail {

template <auto FunPtr, typename Ret, typename... Params>
struct ClosureFunWrapParamRet {
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;
  
  static void call(ClosureData *, pass_type<Params>... params, Ret *ret) noexcept {
    new (ret) Ret{FunPtr(pass_traits<Params>::wrap(params)...)};
  }
};

template <auto FunPtr, typename Ret, typename... Params>
struct ClosureFunWrapNormalRet {
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;
  
  static pass_type<Ret> call(ClosureData *, pass_type<Params>... params) noexcept {
    if constexpr (std::is_void_v<Ret>) {
      FunPtr(pass_traits<Params>::wrap(params)...);
    } else {
      return pass_traits<Ret>::unwrap(
        FunPtr(pass_traits<Params>::wrap(params)...)
      );
    }
  }
};

template <auto FunPtr, typename Ret, typename... Params>
struct FunctionWrapParamRet {
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;
  
  static void call(pass_type<Params>... params, Ret *ret) noexcept {
    new (ret) Ret{FunPtr(pass_traits<Params>::wrap(params)...)};
  }
};

template <auto FunPtr, typename Ret, typename... Params>
struct FunctionWrapNormalRet {
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;
  
  static pass_type<Ret> call(pass_type<Params>... params) noexcept {
    if constexpr (std::is_void_v<Ret>) {
      FunPtr(pass_traits<Params>::wrap(params)...);
    } else {
      return pass_traits<Ret>::unwrap(
        FunPtr(pass_traits<Params>::wrap(params)...)
      );
    }
  }
};

template <auto MemFunPtr, typename Class, typename Ret, typename... Params>
struct MethodWrapParamRet {
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;
  
  static void call(Class &rec, pass_type<Params>... params, Ret *ret) noexcept {
    new (ret) Ret{(rec.*MemFunPtr)(pass_traits<Params>::wrap(params)...)};
  }
};

template <auto MemFunPtr, typename Class, typename Ret, typename... Params>
struct MethodWrapNormalRet {
  template <typename Param>
  using pass_type = typename pass_traits<Param>::type;
  
  static pass_type<Ret> call(Class &rec, pass_type<Params>... params) noexcept {
    if constexpr (std::is_void_v<Ret>) {
      (rec.*MemFunPtr)(pass_traits<Params>::wrap(params)...);
    } else {
      return pass_traits<Ret>::unwrap(
        (rec.*MemFunPtr)(pass_traits<Params>::wrap(params)...)
      );
    }
  }
};

} // detail

template <typename Sig, auto FunPtr>
struct ClosureFunWrap;

template <auto FunPtr, bool Noexcept, typename Ret, typename... Params>
struct ClosureFunWrap<Ret(Params...) noexcept(Noexcept), FunPtr> {
  using type = std::conditional_t<
    pass_traits<Ret>::nontrivial,
    detail::ClosureFunWrapParamRet<FunPtr, Ret, Params...>,
    detail::ClosureFunWrapNormalRet<FunPtr, Ret, Params...>
  >;
};

template <typename Sig, auto FunPtr>
struct FunctionWrap;

template <auto FunPtr, bool Noexcept, typename Ret, typename... Params>
struct FunctionWrap<Ret(*)(Params...) noexcept(Noexcept), FunPtr> {
  using type = std::conditional_t<
    pass_traits<Ret>::nontrivial,
    detail::FunctionWrapParamRet<FunPtr, Ret, Params...>,
    detail::FunctionWrapNormalRet<FunPtr, Ret, Params...>
  >;
};

template <typename Sig, auto FunPtr>
struct MethodWrap;

template <auto FunPtr, bool Noexcept, typename Class, typename Ret, typename... Params>
struct MethodWrap<Ret(Class::*)(Params...) noexcept(Noexcept), FunPtr> {
  using type = std::conditional_t<
    pass_traits<Ret>::nontrivial,
    detail::MethodWrapParamRet<FunPtr, Class, Ret, Params...>,
    detail::MethodWrapNormalRet<FunPtr, Class, Ret, Params...>
  >;
};

template <auto FunPtr, bool Noexcept, typename Class, typename Ret, typename... Params>
struct MethodWrap<Ret(Class::*)(Params...) const noexcept(Noexcept), FunPtr> {
  using type = std::conditional_t<
    pass_traits<Ret>::nontrivial,
    detail::MethodWrapParamRet<FunPtr, const Class, Ret, Params...>,
    detail::MethodWrapNormalRet<FunPtr, const Class, Ret, Params...>
  >;
};


}

#endif
