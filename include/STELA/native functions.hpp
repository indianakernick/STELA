//
//  native functions.hpp
//  STELA
//
//  Created by Indi Kernick on 27/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_native_functions_hpp
#define stela_native_functions_hpp

#include <algorithm>
#include "native types.hpp"
#include "wrap function.hpp"

namespace stela {

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

template <typename Sig>
struct remove_noexcept;

template <bool Noexcept, typename Ret, typename... Params>
struct remove_noexcept<Ret(Params...) noexcept(Noexcept)> {
  using type = Ret(Params...);
};

template <auto FunPtr>
auto makeClosureFromFunc() noexcept {
  using Sig = std::remove_pointer_t<decltype(FunPtr)>;
  using WithoutNoexcept = typename remove_noexcept<Sig>::type;
  return Closure<WithoutNoexcept>{
    ClosureFunWrap<WithoutNoexcept, FunPtr>::type::call
  };
}

}

#endif
