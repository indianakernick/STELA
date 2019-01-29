//
//  reflect decl.hpp
//  STELA
//
//  Created by Indi Kernick on 29/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_reflect_decl_hpp
#define stela_reflect_decl_hpp

#include <tuple>
#include "reflection state.hpp"

namespace stela::bnd {

template <typename... Types>
struct Decls {
  template <typename... DeclTypes>
  Decls(const DeclTypes &... decls) noexcept
    : decls{decls...} {}
  
  void reg(ReflectionState &state) const {
    reflectDecls(state, std::index_sequence_for<Types...>{});
  }

private:
  std::tuple<Types...> decls;
  
  template <size_t Index>
  void reflectDecl(ReflectionState &state) const {
    std::get<Index>(decls).reg(state);
  }
  
  template <size_t... Indicies>
  void reflectDecls(ReflectionState &state, std::index_sequence<Indicies...>) const {
    (reflectDecl<Indicies>(state), ...);
  }
};

template <>
struct Decls<> {
  void reg(ReflectionState &) const {}
};

template <typename... Types>
Decls(const Types &...) -> Decls<Types...>;

/// A method implemented as a member function pointer
template <auto MemFunPtr>
struct Method {
  std::string_view name;
  
  void reg(ReflectionState &state) const {
    state.reflectMethod<MemFunPtr>(name);
  }
};

template <auto FunPtr, bool Method = true>
struct Func {
  std::string_view name;
  
  void reg(ReflectionState &state) const noexcept {
    state.reflectFunc<FunPtr, Method>(name);
  }
};

template <typename Sig, bool Method = true>
struct PlainFunc {
  std::string_view name;
  Sig *func;
  
  void reg(ReflectionState &state) const noexcept {
    state.reflectPlainFunc<Method>(name, func);
  }
};

template <bool Method>
struct method_tag_t {};

template <bool Method>
constexpr method_tag_t<Method> method_tag {};

template <typename Sig>
PlainFunc(std::string_view, Sig *) -> PlainFunc<Sig>;

template <typename Sig, bool Method>
PlainFunc(std::string_view, Sig *, method_tag_t<Method>) -> PlainFunc<Sig, Method>;

template <typename Type, bool Strong = true>
struct Typealias {
  std::string_view name;
  
  void reg(ReflectionState &state) const {
    auto alias = make_retain<ast::TypeAlias>();
    alias->name = name;
    alias->type = state.getType<Type>();
    alias->strong = Strong;
    state.reflectDecl(alias);
  }
};

template <typename Type>
ast::LitrPtr makeLiteral(Type);

template <>
inline ast::LitrPtr makeLiteral(const std::string_view value) {
  auto con = make_retain<ast::StringLiteral>();
  con->value = value;
  return con;
}

template <>
inline ast::LitrPtr makeLiteral(const std::string &value) {
  auto con = make_retain<ast::StringLiteral>();
  con->value = value;
  return con;
}

#define NUMBER_LITERAL(TYPE)                                                    \
  template <>                                                                   \
  inline ast::LitrPtr makeLiteral(const TYPE value) {                           \
    auto litr = make_retain<ast::NumberLiteral>();                              \
    litr->value = value;                                                        \
    return litr;                                                                \
  }

NUMBER_LITERAL(Byte);
NUMBER_LITERAL(Char);
NUMBER_LITERAL(Real);
NUMBER_LITERAL(Sint);
NUMBER_LITERAL(Uint);

#undef NUMBER_LITERAL

template <>
inline ast::LitrPtr makeLiteral(const Bool value) {
  auto litr = make_retain<ast::BoolLiteral>();
  litr->value = value;
  return litr;
}

template <typename Type>
struct Constant {
  Constant(const std::string_view name, const Bool value)
    : name{name}, value{value} {}

  const std::string_view name;
  const Type value;
  
  void reg(ReflectionState &state) const {
    state.reflectConstant(name, makeLiteral(value));
  }
};

template <typename Type>
Constant(std::string_view, Type) -> Constant<Type>;

template <typename Enum>
struct EnumConstant {
  const std::string_view name;
  const Enum value;
  
  void reg(ReflectionState &state) const {
    using Underlying = std::underlying_type_t<Enum>;
    auto cast = make_retain<ast::Make>();
    cast->expr = makeLiteral(static_cast<Underlying>(value));
    cast->type = state.getType<Enum>();
    state.reflectConstant(name, std::move(cast));
  }
};

template <typename Type>
EnumConstant(std::string_view, Type) -> EnumConstant<Type>;

}

#endif
