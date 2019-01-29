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
#include <type_traits>
#include "retain ptr.hpp"
#include "pass traits.hpp"

namespace llvm {

class ExecutionEngine;

}

namespace stela {

/// A wrapper around a compiled stela function. Acts as an ABI adapter to call
/// Stela functions using the Stela ABI
template <typename Fun, bool Method = false>
class Function;

template <bool Method, typename Ret, typename... Params>
class Function<Ret(Params...), Method> {
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
      if constexpr (Method) {
        ptr(unwrap<Params, Indicies>(params)..., retPtr);
      } else {
        ptr(nullptr, unwrap<Params, Indicies>(params)..., retPtr);
      }
      Ret retObj{std::move(*retPtr)};
      retPtr->~Ret();
      return retObj;
    } else if constexpr (std::is_void_v<Ret>) {
      if constexpr (Method) {
        ptr(unwrap<Params, Indicies>(params)...);
      } else {
        ptr(nullptr, unwrap<Params, Indicies>(params)...);
      }
    } else {
      if constexpr (Method) {
        return pass_traits<Ret>::wrap(ptr(unwrap<Params, Indicies>(params)...));
      } else {
        return pass_traits<Ret>::wrap(ptr(nullptr, unwrap<Params, Indicies>(params)...));
      }
    }
  }

public:
  static constexpr bool param_ret = pass_traits<Ret>::nontrivial;

  using type = std::conditional_t<
    Method,
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

template <typename Type>
class Global {
public:
  using type = Type;

  explicit Global(const uint64_t addr) noexcept
    : ptr{reinterpret_cast<type *>(addr)} {}
  
  type &operator*() const noexcept {
    return *ptr;
  }
  type *operator->() const noexcept {
    return ptr;
  }

private:
  type *ptr;
};

uint64_t getFunctionAddress(llvm::ExecutionEngine *, const std::string &);

template <typename Sig, bool Method = false>
auto getFunc(llvm::ExecutionEngine *engine, const std::string &name) {
  return Function<Sig, Method>{getFunctionAddress(engine, name)};
}

uint64_t getGlobalAddress(llvm::ExecutionEngine *, const std::string &);

template <typename Type>
auto getGlobal(llvm::ExecutionEngine *engine, const std::string &name) {
  return Global<Type>{getGlobalAddress(engine, name)};
}

}

#endif
