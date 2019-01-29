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
#include "reflector.hpp"

namespace stela {

template <typename... Types>
struct Decls {
  template <typename... DeclTypes>
  Decls(const DeclTypes &... decls) noexcept
    : decls{decls...} {}
  
  void reg(Reflector &refl) const {
    reflectDecls(refl, std::index_sequence_for<Types...>{});
  }

private:
  std::tuple<Types...> decls;
  
  template <size_t Index>
  void reflectDecl(Reflector &refl) const {
    std::get<Index>(decls).reg(refl);
  }
  
  template <size_t... Indicies>
  void reflectDecls(Reflector &refl, std::index_sequence<Indicies...>) const {
    (reflectDecl<Indicies>(refl), ...);
  }
};

template <>
struct Decls<> {
  void reg(Reflector &) const {}
};

template <typename... Types>
Decls(const Types &...) -> Decls<Types...>;

/// A method implemented as a member function pointer
template <auto MemFunPtr>
struct Method {
  std::string_view name;
  
  void reg(Reflector &refl) const {
    refl.reflectMethod<MemFunPtr>(name);
  }
};

/// A method implemented as a function pointer
template <auto FunPtr>
struct MethodFunc {
  std::string_view name;
  
  void reg(Reflector &refl) const noexcept {
    refl.reflectFunc<FunPtr, true>(name);
  }
};

template <typename Sig>
struct MethodPlain {
  std::string_view name;
  Sig *func;
  
  void reg(Reflector &refl) const noexcept {
    refl.reflectPlainFunc<true>(name, func);
  }
};

template <typename Sig>
MethodPlain(std::string_view, Sig *) -> MethodPlain<Sig>;

template <typename Type, bool Strong = true>
struct Typealias {
  std::string_view name;
  
  void reg(Reflector &refl) const {
    auto alias = make_retain<ast::TypeAlias>();
    alias->name = name;
    alias->type = refl.getType<Type>();
    alias->strong = Strong;
    refl.reflectDecl(alias);
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
  
  void reg(Reflector &refl) const {
    refl.reflectConstant(name, makeLiteral(value));
  }
};

template <typename Type>
Constant(std::string_view, Type) -> Constant<Type>;

template <typename Enum>
struct EnumConstant {
  const std::string_view name;
  const Enum value;
  
  void reg(Reflector &refl) const {
    using Underlying = std::underlying_type_t<Enum>;
    auto cast = make_retain<ast::Make>();
    cast->expr = makeLiteral(static_cast<Underlying>(value));
    cast->type = refl.getType<Enum>();
    refl.reflectConstant(name, std::move(cast));
  }
};

template <typename Type>
EnumConstant(std::string_view, Type) -> EnumConstant<Type>;

}

#endif
