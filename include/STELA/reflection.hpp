//
//  reflection.hpp
//  STELA
//
//  Created by Indi Kernick on 26/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_reflection_hpp
#define stela_reflection_hpp

#include "reflect decl.hpp"
#include "reflect type.hpp"

namespace stela {

template <typename Type>
struct reflect<Type, std::void_t<detail::basic_reflectible<Type>>> : detail::with_decls<Type> {
  static constexpr std::string_view reflected_name = Type::reflected_name;
  static inline const auto &reflected_type = Type::reflected_type;
};

template <typename Elem>
struct reflect<Array<Elem>> {
  static constexpr std::string_view reflected_name = "";
  static inline const auto reflected_type = bnd::Array<Elem>{};
};

template <typename Sig>
struct reflect<Closure<Sig>> {
  static constexpr std::string_view reflected_name = "";
  static inline const auto reflected_type = bnd::Closure<Sig>{};
};

#define REFLECT_PRIMITIVE(TYPE)                                                 \
  template <>                                                                   \
  struct reflect<TYPE> {                                                        \
    static constexpr std::string_view reflected_name = "";                      \
    static inline const auto reflected_type = bnd::Primitive<TYPE>{};           \
  }

REFLECT_PRIMITIVE(Void);
REFLECT_PRIMITIVE(Opaq);
REFLECT_PRIMITIVE(Bool);
REFLECT_PRIMITIVE(Byte);
REFLECT_PRIMITIVE(Char);
REFLECT_PRIMITIVE(Real);
REFLECT_PRIMITIVE(Sint);
REFLECT_PRIMITIVE(Uint);

#undef REFLECT_PRIMITIVE

#define STELA_REFLECT_NAME(CLASS, NAME)                                         \
  using reflected_typedef = CLASS;                                              \
  static constexpr std::string_view reflected_name = #NAME
#define STELA_REFLECT(CLASS) STELA_REFLECT_NAME(CLASS, CLASS)
#define STELA_REFLECT_ANON(CLASS) STELA_REFLECT_NAME(CLASS,)

#define STELA_THIS reflected_typedef

#define STELA_PRIMITIVE(TYPE)                                                   \
  static inline const auto reflected_type = stela::bnd::Primitive<TYPE>{}
#define STELA_CLASS(...)                                                        \
  static inline const auto reflected_type = stela::bnd::Class{                  \
    stela::bnd::class_tag<reflected_typedef>, __VA_ARGS__                       \
  }
#define STELA_FIELD(MEMBER)                                                     \
  stela::bnd::Field<&reflected_typedef::MEMBER>{#MEMBER}
#define STELA_ENUM_TYPE()                                                       \
  static inline const auto reflected_type = stela::bnd::Primitive<              \
    std::underlying_type_t<reflected_typedef>                                   \
  >{}
#define STELA_ARRAY(ELEM)                                                       \
  static inline const auto reflected_type = stela::bnd::Array<ELEM>{}
#define STELA_CLOSURE(SIGNATURE)                                                \
  static inline const auto reflected_type = stela::bnd::Closure<SIGNATURE>{}

#define STELA_DECLS(...)                                                        \
  static inline const auto reflected_decl = stela::bnd::Decls{__VA_ARGS__}

#define STELA_METHOD_NAME(MEMBER, NAME)                                         \
  stela::bnd::Method<&reflected_typedef::MEMBER>{#NAME}
#define STELA_METHOD(MEMBER)                                                    \
  STELA_METHOD_NAME(MEMBER, MEMBER)
#define STELA_METHOD_NAME_SIG(MEMBER, NAME, SIGNATURE)                          \
  stela::bnd::Method<stela::overload_mem<SIGNATURE>(&reflected_typedef::MEMBER)>{#NAME}
#define STELA_METHOD_SIG(MEMBER, SIGNATURE)                                     \
  STELA_METHOD_NAME_SIG(MEMBER, MEMBER, SIGNATURE)
  
#define STELA_METHOD_FUNC_NAME(FUNC, NAME)                                      \
  stela::bnd::Func<&FUNC>{#NAME}
#define STELA_METHOD_FUNC(FUNC)                                                 \
  STELA_METHOD_FUNC_NAME(FUNC, FUNC)
#define STELA_METHOD_FUNC_NAME_SIG(FUNC, NAME, SIGNATURE)                       \
  stela::bnd::Func<stela::overload<SIGNATURE>(&FUNC)>{#NAME}
#define STELA_METHOD_FUNC_SIG(FUNC, SIGNATURE)                                  \
  STELA_METHOD_FUNC_NAME_SIG(FUNC, FUNC, SIGNATURE)

#define STELA_METHOD_PLAIN_NAME(NAME, ...)                                      \
  stela::bnd::PlainFunc{#NAME, __VA_ARGS__}
#define STELA_METHOD_PLAIN(FUNC)                                                \
  stela::bnd::PlainFunc{#FUNC, FUNC}

#define STELA_TYPEALIAS(NAME, TYPE)                                             \
  stela::bnd::Typealias<Type>{NAME}
#define STELA_CONSTANT(NAME, VALUE)                                             \
  stela::bnd::Constant{NAME, VALUE}
#define STELA_ENUM_CASE(NAME, VALUE)                                            \
  stela::bnd::EnumConstant{#NAME, reflected_typedef::VALUE}

}

#endif
