//
//  native types.hpp
//  STELA
//
//  Created by Indi Kernick on 27/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_native_types_hpp
#define stela_native_types_hpp

#include "number.hpp"
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

struct ClosureData : ref_count {
  ~ClosureData() {
    dtor(this);
  }
  
  // uint64_t ref
  void (*dtor)(void *);
};

template <typename Fun, bool Method>
class Function;

template <typename Fun>
struct Closure;

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

}

#endif
