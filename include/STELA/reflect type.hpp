//
//  reflect type.hpp
//  STELA
//
//  Created by Indi Kernick on 29/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_reflect_type_hpp
#define stela_reflect_type_hpp

#include <new>
#include "reflection state.hpp"

namespace stela::bnd {

template <typename Type>
struct Primitive;

#define PRIMITIVE(TYPE)                                                         \
  template <>                                                                   \
  struct Primitive<TYPE> {                                                      \
    static ast::TypePtr get(ReflectionState &) {                                \
      return make_retain<ast::BtnType>(ast::BtnTypeEnum::TYPE);                 \
    }                                                                           \
  }

PRIMITIVE(Void);
PRIMITIVE(Opaq);
PRIMITIVE(Bool);
PRIMITIVE(Byte);
PRIMITIVE(Char);
PRIMITIVE(Real);
PRIMITIVE(Sint);
PRIMITIVE(Uint);

#undef PRIMITIVE

template <auto MemPtr>
struct Field {
  const std::string_view name;
  static constexpr auto member = MemPtr;
};

namespace detail {

#define WRITE(CTOR, TRAIT)                                                      \
  if constexpr (std::is_trivially_##TRAIT##_v<Class>) {                         \
    CTOR.addr = ast::UserCtor::trivial;                                         \
  } else if constexpr (!std::is_##TRAIT##_v<Class>) {                           \
    CTOR.addr = ast::UserCtor::none;                                            \
  } else
#define WRITE_UNARY(CTOR, TRAIT, NAME, EXPR)                                    \
  WRITE(CTOR, TRAIT) {                                                          \
    CTOR.addr = reinterpret_cast<uint64_t>(+[] (Class *obj) noexcept {          \
      EXPR;                                                                     \
    });                                                                         \
    CTOR.name = mangledName(className + "_"#NAME);                              \
  }
#define WRITE_BINARY(CTOR, TRAIT, NAME, EXPR)                                   \
  WRITE(CTOR, TRAIT) {                                                          \
    CTOR.addr = reinterpret_cast<uint64_t>(+[] (Class *dst, Class *src) noexcept { \
      EXPR;                                                                     \
    });                                                                         \
    CTOR.name = mangledName(className + "_"#NAME);                              \
  }

template <typename Class>
auto writeEq(ast::UserCtor &ctor, const std::string &className, int) noexcept
  -> std::void_t<decltype(std::declval<Class>() == std::declval<Class>())> {
  ctor.addr = reinterpret_cast<uint64_t>(+[] (Class *lhs, Class *rhs) noexcept -> Bool {
    return *lhs == *rhs;
  });
  ctor.name = mangledName(className + "_eq");
}
template <typename Class>
static void writeEq(ast::UserCtor &ctor, const std::string &, long) {
  ctor.addr = ast::UserCtor::none;
}

template <typename Class>
auto writeLt(ast::UserCtor &ctor, const std::string &className, int) noexcept
  -> std::void_t<decltype(std::declval<Class>() < std::declval<Class>())> {
  ctor.addr = reinterpret_cast<uint64_t>(+[] (Class *lhs, Class *rhs) noexcept -> Bool {
    return *lhs < *rhs;
  });
  ctor.name = mangledName(className + "_lt");
}
template <typename ClassType>
static void writeLt(ast::UserCtor &ctor, const std::string &, long) noexcept {
  ctor.addr = ast::UserCtor::none;
}

template <typename Class>
auto writeBool(ast::UserCtor &ctor, const std::string &className, int) noexcept
  -> std::void_t<decltype(static_cast<bool>(std::declval<Class>()))> {
  ctor.addr = reinterpret_cast<uint64_t>(+[] (Class *obj) noexcept -> Bool {
    return bool(*obj);
  });
  ctor.name = mangledName(className + "_bool");
}
template <typename Class>
static void writeBool(ast::UserCtor &ctor, const std::string &, long) noexcept {
  ctor.addr = ast::UserCtor::none;
}

template <typename Class>
void writeCtors(ast::UserType &user) noexcept {
  constexpr std::string_view classNameView = reflect<Class>::reflected_name;
  std::string className;
  if constexpr (classNameView.empty()) {
    className = "anon";
  } else {
    className.assign(classNameView.data(), classNameView.size());
  }
  WRITE_UNARY(user.dtor, destructible, dtor, obj->~Class())
  WRITE_UNARY(user.defCtor, default_constructible, def_ctor, new (obj) Class{})
  WRITE_BINARY(user.copCtor, copy_constructible, cop_ctor, new (dst) Class{*src})
  WRITE_BINARY(user.copAsgn, copy_assignable, cop_asgn, *dst = *src)
  WRITE_BINARY(user.movCtor, move_constructible, mov_ctor, new (dst) Class{std::move(*src)})
  WRITE_BINARY(user.movAsgn, move_assignable, mov_asgn, *dst = std::move(*src))
  writeEq<Class>(user.eq, className, 0);
  writeLt<Class>(user.lt, className, 0);
  writeBool<Class>(user.boolConv, className, 0);
}

#undef WRITE_BINARY
#undef WRITE_UNARY
#undef WRITE

template <typename Class>
void writeUserType(ast::UserType &user) noexcept {
  static_assert(sizeof(Class) % alignof(Class) == 0);
  user.size = sizeof(Class);
  user.align = alignof(Class);
  writeCtors<Class>(user);
}

} // namespace detail

template <typename T>
struct class_tag_t {};

template <typename T>
constexpr class_tag_t<T> class_tag {};

template <typename Type, typename... MemTypes>
struct Class {
  template <typename... FieldTypes>
  Class(class_tag_t<Type>, FieldTypes... fields) noexcept
    : names{fields.name...}, offsets{getOffset(fields.member)...} {}
  
  ast::TypePtr get(ReflectionState &state) const noexcept {
    auto user = make_retain<ast::UserType>();
    detail::writeUserType<Type>(*user);
    user->fields.reserve(sizeof...(MemTypes));
    pushFields(user->fields, state, std::make_index_sequence<sizeof...(MemTypes)>{});
    return user;
  }

private:
  const std::string_view names[sizeof...(MemTypes)];
  const size_t offsets[sizeof...(MemTypes)];
  
  template <typename MemType, size_t Index>
  ast::UserField getField(ReflectionState &state) const noexcept {
    return {names[Index], state.getType<MemType>(), offsets[Index]};
  }
  
  template <size_t... Indicies>
  void pushFields(ast::UserFields &fields, ReflectionState &state, std::index_sequence<Indicies...>) const noexcept {
    (fields.push_back(getField<MemTypes, Indicies>(state)), ...);
  }
};

template <typename Type>
struct Class<Type> {
  Class(class_tag_t<Type>) {}

  ast::TypePtr get(ReflectionState &) const noexcept {
    auto user = make_retain<ast::UserType>();
    detail::writeUserType<Type>(*user);
    return user;
  }
};

template <typename Type, auto... MemPtrs>
Class(class_tag_t<Type>, Field<MemPtrs>...) -> Class<
  Type,
  typename member_traits<decltype(MemPtrs)>::member...
>;

template <typename Elem>
struct Array {
  ast::TypePtr get(ReflectionState &state) const noexcept {
    auto array = make_retain<ast::ArrayType>();
    array->elem = state.getType<Elem>();
    return array;
  }
};

template <typename Sig>
struct Closure {
  ast::TypePtr get(ReflectionState &state) const noexcept {
    auto closure = make_retain<ast::FuncType>();
    detail::write_sig<Sig *, false>::write(*closure, state);
    return closure;
  }
};

}

#endif
