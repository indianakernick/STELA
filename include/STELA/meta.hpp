//
//  meta.hpp
//  STELA
//
//  Created by Indi Kernick on 28/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_meta_hpp
#define stela_meta_hpp

#include <type_traits>

namespace stela {

template <typename MemPtr>
struct member_traits;

template <typename Class, typename Member>
struct member_traits<Member (Class::*)> {
  using object = Class;
  using member = Member;
};

namespace detail {

struct Dummy {
  int *x;
  
  char fun0() const;
  unsigned fun1(double);
  long fun2(int, short) noexcept;
  
  char fun() const;
  unsigned fun(double);
  long fun(int, short) noexcept;
};

}

static_assert(std::is_same_v<
  typename member_traits<decltype(&detail::Dummy::x)>::object,
  detail::Dummy
>);
static_assert(std::is_same_v<
  typename member_traits<decltype(&detail::Dummy::x)>::member,
  int *
>);

namespace detail {

template <typename... Types>
struct first_type;

template <typename First, typename... Rest>
struct first_type<First, Rest...> {
  using type = First;
};

}

template <typename... Types>
using first_t = typename detail::first_type<Types...>::type;

static_assert(std::is_same_v<first_t<int, char, long, double>, int>);
static_assert(std::is_same_v<first_t<int, char, long>, int>);
static_assert(std::is_same_v<first_t<int, char>, int>);
static_assert(std::is_same_v<first_t<int>, int>);

template <typename Fun>
struct fun_to_mem;

template <bool Noexcept, typename Ret, typename Class, typename... Params>
struct fun_to_mem<Ret(*)(Class, Params...) noexcept(Noexcept)> {
  static_assert(std::is_reference_v<Class>);
  using real_class = std::remove_cv_t<std::remove_reference_t<Class>>;
  using type = std::conditional_t<
    std::is_const_v<std::remove_reference_t<Class>>,
    Ret(real_class::*)(Params...) const noexcept(Noexcept),
    Ret(real_class::*)(Params...)       noexcept(Noexcept)
  >;
};

static_assert(std::is_same_v<
  typename fun_to_mem<char(*)(const detail::Dummy &)>::type,
  decltype(&detail::Dummy::fun0)
>);
static_assert(std::is_same_v<
  typename fun_to_mem<unsigned(*)(detail::Dummy &, double)>::type,
  decltype(&detail::Dummy::fun1)
>);
static_assert(std::is_same_v<
  typename fun_to_mem<long(*)(detail::Dummy &, int, short) noexcept>::type,
  decltype(&detail::Dummy::fun2)
>);

template <typename Mem>
struct mem_to_fun;

template <bool Noexcept, typename Ret, typename Class, typename... Params>
struct mem_to_fun<Ret(Class::*)(Params...) noexcept(Noexcept)> {
  using type = Ret(*)(Class &, Params...) noexcept(Noexcept);
};

template <bool Noexcept, typename Ret, typename Class, typename... Params>
struct mem_to_fun<Ret(Class::*)(Params...) const noexcept(Noexcept)> {
  using type = Ret(*)(const Class &, Params...) noexcept(Noexcept);
};

static_assert(std::is_same_v<
  typename mem_to_fun<decltype(&detail::Dummy::fun0)>::type,
  char(*)(const detail::Dummy &)
>);
static_assert(std::is_same_v<
  typename mem_to_fun<decltype(&detail::Dummy::fun1)>::type,
  unsigned(*)(detail::Dummy &, double)
>);
static_assert(std::is_same_v<
  typename mem_to_fun<decltype(&detail::Dummy::fun2)>::type,
  long(*)(detail::Dummy &, int, short) noexcept
>);

template <typename Sig>
constexpr auto overload_mem(typename fun_to_mem<Sig *>::type mem) {
  return mem;
}

static_assert(
  overload_mem<char(const detail::Dummy &)>(&detail::Dummy::fun) ==
  static_cast<char(detail::Dummy::*)() const>(&detail::Dummy::fun)
);
static_assert(
  overload_mem<unsigned(detail::Dummy &, double)>(&detail::Dummy::fun) ==
  static_cast<unsigned(detail::Dummy::*)(double)>(&detail::Dummy::fun)
);
static_assert(
  overload_mem<long(detail::Dummy &, int, short) noexcept>(&detail::Dummy::fun) ==
  static_cast<long(detail::Dummy::*)(int, short) noexcept>(&detail::Dummy::fun)
);

template <typename Sig>
constexpr auto overload(Sig *fun) {
  return fun;
}

namespace detail {

char dummyFun();
unsigned dummyFun(double);
long dummyFun(int, short) noexcept;

}

static_assert(
  overload<char()>(detail::dummyFun) ==
  static_cast<char(*)()>(detail::dummyFun)
);
static_assert(
  overload<unsigned(double)>(detail::dummyFun) ==
  static_cast<unsigned(*)(double)>(detail::dummyFun)
);
static_assert(
  overload<long(int, short)>(detail::dummyFun) ==
  static_cast<long(*)(int, short)>(detail::dummyFun)
);

template <typename Class, typename Member>
size_t getOffset(Member (Class::*member)) {
  Class *const null = nullptr;
  Member *const pointer = &(null->*member);
  return reinterpret_cast<size_t>(pointer);
}

template <typename Family>
struct TypeID {
private:
  static inline size_t counter = 0;

public:
  template <typename Type>
  static inline const size_t id = counter++;
};

}

#endif
