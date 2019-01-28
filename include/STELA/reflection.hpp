//
//  reflection.hpp
//  STELA
//
//  Created by Indi Kernick on 26/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_reflection_hpp
#define stela_reflection_hpp

#include <new>
#include "meta.hpp"
#include "symbols.hpp"
#include "pass traits.hpp"
#include "wrap function.hpp"

namespace stela {

std::string mangledName(std::string_view);
std::string mangledName(const std::string &);

template <typename Type, typename = void>
struct reflect {
  //static_assert(sizeof(Type) == 0, "Type is not reflectible");
};

namespace detail {

template <typename Type>
constexpr auto hasDecls(int) -> decltype(Type::reflected_decl, bool{}) {
  return true;
}

template <typename Type>
constexpr bool hasDecls(long) {
  return false;
}

template <typename Type>
using basic_reflectible = decltype(Type::reflected_name, Type::reflected_type);

struct DeclDummy {
  static constexpr std::string_view reflected_name = "steve";
  static inline const auto reflected_type = 0;
  static inline const auto reflected_decl = 1;
};

struct NoDeclDummy {
  static constexpr std::string_view reflected_name = "steve";
  static inline const auto reflected_type = 0;
};

static_assert(hasDecls<DeclDummy>(0));
static_assert(!hasDecls<NoDeclDummy>(0));

template <typename Type, typename = void>
struct with_decls {};

template <typename Type>
struct with_decls<Type, std::enable_if_t<hasDecls<Type>(0)>> {
  static inline const auto &reflected_decl = Type::reflected_decl;
};

}

template <typename Type>
struct reflect<Type, std::void_t<detail::basic_reflectible<Type>>> : detail::with_decls<Type> {
  static constexpr std::string_view reflected_name = Type::reflected_name;
  static inline const auto &reflected_type = Type::reflected_type;
};

namespace detail {

template <typename Signature, bool Method>
struct write_sig;

}

class Reflector {
public:
  template <typename Type>
  void reflectType() {
    const size_t index = typeIndex<Type>();
    if (index < types.size()) {
      if (types[index]) return;
    } else {
      types.insert(types.end(), index - types.size() + 1, nullptr);
    }
    
    using Refl = reflect<Type>;
    constexpr std::string_view name = Refl::reflected_name;
    
    if constexpr (name.empty()) {
      types[index] = Refl::reflected_type.get(*this);
    } else {
      auto alias = make_retain<ast::TypeAlias>();
      alias->name = name;
      alias->type = Refl::reflected_type.get(*this);
      alias->strong = true;
      auto named = make_retain<ast::NamedType>();
      named->name = name;
      named->definition = alias.get();
      decls.push_back(std::move(alias));
      types[index] = std::move(named);
    }
    if constexpr (detail::hasDecls<Refl>(0)) {
      Refl::reflected_decl.reg(*this);
    }
  }
  
  /// Builds a wrapper function with a direct call to the given function
  template <auto FunPtr, bool Method = false>
  void reflectFunc(const std::string_view name) {
    auto func = make_retain<ast::ExtFunc>();
    func->name = name;
    func->mangledName = mangledName(name);
    detail::write_sig<decltype(FunPtr), Method>::write(*func, *this);
    func->impl = reinterpret_cast<uint64_t>( // Clang bug workaround :(
      &FunctionWrap<decltype(FunPtr), static_cast<decltype(FunPtr)>(FunPtr)>::type::call
    );
    reflectDecl(std::move(func));
  }
  
  /// Builds a wrapper function with a direct call to the given member function
  template <auto MemFunPtr>
  void reflectMethod(const std::string_view name) {
    auto func = make_retain<ast::ExtFunc>();
    func->name = name;
    func->mangledName = mangledName(name);
    using Ptr = decltype(MemFunPtr);
    detail::write_sig<typename mem_to_fun<Ptr>::type, true>::write(*func, *this);
    func->impl = reinterpret_cast<uint64_t>( // Clang bug workaround :(
      &MethodWrap<Ptr, static_cast<Ptr>(MemFunPtr)>::type::call
    );
    reflectDecl(std::move(func));
  }
  
  /*
  @TODO Use a closure
  /// Builds a wrapper function with a call to the given function pointer.
  template <bool Method = false, typename Sig>
  void reflectFunc(const std::string_view name, Sig *impl) {
    auto func = make_retain<ast::ExtFunc>();
    func->name = name;
    func->mangledName = mangledName(name);
    detail::write_sig<Sig *, Method>::write(*func, *this);
    func->impl = reinterpret_cast<uint64_t>(
      &FunctionPtrWrap<Sig *>::type::call
    );
    reflectFunc(std::move(func));
  }*/
  
  /// Creates a new dynamic symbol entry that points directly do the given
  /// function. The function is not wrapped so may not work when passing classes.
  template <bool Method = false, typename Sig>
  void reflectPlainFunc(const std::string_view name, Sig *impl) {
    auto func = make_retain<ast::ExtFunc>();
    func->name = name;
    func->mangledName = mangledName(name);
    detail::write_sig<Sig *, Method>::write(*func, *this);
    func->impl = reinterpret_cast<uint64_t>(impl);
    reflectDecl(std::move(func));
  }
  
  /// Provide the symbol name of a C function and all calls will be direct.
  /// The optimizer will be able to reason about the calls.
  /// sqrtf(25.0f) is optimized to 5.0f because the optimizer understands sqrtf
  template <auto FunPtr, bool Method = false>
  void reflectPlainCFunc(const std::string_view name, const std::string_view mangled) {
    auto func = make_retain<ast::ExtFunc>();
    func->name = name;
    func->mangledName = mangled;
    detail::write_sig<decltype(FunPtr), Method>::write(*func, *this);
    reflectDecl(std::move(func));
  }
  
  template <auto FunPtr, bool Method = false>
  void reflectPlainCFunc(const std::string_view name) {
    return reflectPlainCFunc<FunPtr, Method>(name, name);
  }
  
  void reflectConstant(const std::string_view name, ast::ExprPtr init) {
    auto constant = make_retain<ast::Let>();
    constant->name = name;
    constant->expr = init;
    reflectDecl(constant);
  }
  
  void reflectDecl(ast::DeclPtr decl) {
    decls.push_back(decl);
  }

  template <typename Type>
  ast::TypePtr getType() {
    const size_t index = typeIndex<Type>();
    reflectType<Type>();
    return types[index];
  }
  
  void appendDeclsTo(std::vector<ast::DeclPtr> &otherDecls) {
    otherDecls.insert(otherDecls.end(), decls.cbegin(), decls.cend());
  }
  
private:
  std::vector<ast::TypePtr> types;
  std::vector<ast::DeclPtr> decls;
  
  static inline size_t counter = 0;
  
  template <typename T>
  static size_t typeIndex() {
    static const size_t index = counter++;
    return index;
  }
};

namespace detail {

template <typename Param>
ast::ParamType getParamType(Reflector &refl) {
  using WithoutRef = std::remove_reference_t<Param>;
  using WithoutConst = std::remove_const_t<WithoutRef>;
  ast::ParamRef paramRef;
  if constexpr (std::is_same_v<Param, WithoutRef>) {
    paramRef = ast::ParamRef::val;
  } else {
    paramRef = ast::ParamRef::ref;
  }
  return {paramRef, refl.getType<WithoutConst>()};
}

template <bool Noexcept, typename Class, typename Ret, typename... Params>
struct write_sig<Ret(*)(Class, Params...) noexcept(Noexcept), true> {
  static void write(ast::ExtFunc &func, Reflector &refl) {
    func.receiver = getParamType<Class>(refl);
    write_sig<Ret(*)(Params...) noexcept(Noexcept), false>::write(func, refl);
  }
};

template <bool Noexcept, typename Ret, typename... Params>
struct write_sig<Ret(*)(Params...) noexcept(Noexcept), false> {
  static void write(ast::ExtFunc &func, Reflector &refl) {
    static_assert(!std::is_reference_v<Ret>, "Stela functions cannot return references");
    func.ret = refl.getType<Ret>();
    func.params.reserve(sizeof...(Params));
    (func.params.push_back(getParamType<Params>(refl)), ...);
  }
};

}

template <typename Elem>
struct reflect<Array<Elem>> {
  static ast::TypePtr get(Reflector &refl) {
    auto array = make_retain<ast::ArrayType>();
    array->elem = refl.getType<Elem>();
    return array;
  }
};

template <auto Enum>
struct PrimitiveType {
  static ast::TypePtr get(Reflector &) {
    return make_retain<ast::BtnType>(Enum);
  }
};

template <typename Type>
struct Primitive;

#define PRIMITIVE(TYPE)                                                         \
  template <>                                                                   \
  struct Primitive<TYPE> {                                                      \
    static ast::TypePtr get(Reflector &) {                                      \
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

/*template <typename Type>
struct field {
  const std::string_view name;
};

template <typename... Fields>
struct aggregate {
  template <typename... FieldTypes>
  aggregate(FieldTypes... fields) noexcept
    : names{fields.name...} {}

  const std::string_view names[sizeof...(Fields)];
  
  template <typename Field, size_t Index>
  ast::Field getField(Reflector &refl) const noexcept {
    return {names[Index], refl.getNode<Field>(), {}};
  }
  
  template <size_t... Indicies>
  void pushFields(ast::Fields &fields, Reflector &refl, std::index_sequence<Indicies...>) const noexcept {
    (fields.push_back(getField<Fields, Indicies>(refl)), ...);
  }
  
  ast::TypePtr get(Reflector &refl) const noexcept {
    auto strut = make_retain<ast::StructType>();
    strut->fields.reserve(sizeof...(Fields));
    pushFields(strut->fields, refl, std::index_sequence_for<Fields...>{});
    return strut;
  }
};

template <typename... Fields>
aggregate(field<Fields>...) -> aggregate<Fields...>;
*/

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

template <typename Type, typename... MemTypes>
struct Class {
  template <typename... FieldTypes>
  Class(class_tag_t<Type>, FieldTypes... fields) noexcept
    : names{fields.name...}, offsets{getOffset(fields.member)...} {}
  
  ast::TypePtr get(Reflector &refl) const noexcept {
    auto user = make_retain<ast::UserType>();
    detail::writeUserType<Type>(*user);
    user->fields.reserve(sizeof...(MemTypes));
    pushFields(user->fields, refl, std::make_index_sequence<sizeof...(MemTypes)>{});
    return user;
  }

private:
  const std::string_view names[sizeof...(MemTypes)];
  const size_t offsets[sizeof...(MemTypes)];
  
  template <typename MemType, size_t Index>
  ast::UserField getField(Reflector &refl) const noexcept {
    return {names[Index], refl.getType<MemType>(), offsets[Index]};
  }
  
  template <size_t... Indicies>
  void pushFields(ast::UserFields &fields, Reflector &refl, std::index_sequence<Indicies...>) const noexcept {
    (fields.push_back(getField<MemTypes, Indicies>(refl)), ...);
  }
};

template <typename Type>
struct Class<Type> {
  Class(class_tag_t<Type>) {}

  ast::TypePtr get(Reflector &) const noexcept {
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

template <auto MemFunPtr>
struct Method {
  std::string_view name;
  
  void reg(Reflector &refl) const {
    refl.reflectMethod<MemFunPtr>(name);
  }
};

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

#define REFLECT_PRIMITIVE(TYPE)                                                 \
  template <>                                                                   \
  struct reflect<TYPE> {                                                        \
    static constexpr std::string_view reflected_name = "";                      \
    static inline const auto reflected_type = Primitive<TYPE>{};                \
    static inline const auto reflected_decl = Decls{};                          \
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

#define STELA_PRIMITIVE(TYPE)                                                   \
  static inline const auto reflected_type = stela::Primitive<TYPE>{}

#define STELA_CLASS(...)                                                        \
  static inline const auto reflected_type = stela::Class{                       \
    class_tag_t<reflected_typedef>{}, __VA_ARGS__                               \
  }

#define STELA_ENUM_TYPE()                                                       \
  static inline const auto reflected_type = stela::Primitive<                   \
    std::underlying_type_t<reflected_typedef>                                   \
  >{}

#define STELA_FIELD(MEMBER)                                                     \
  stela::Field<&reflected_typedef::MEMBER>{#MEMBER}

#define STELA_DECLS(...)                                                        \
  static inline const auto reflected_decl = stela::Decls{__VA_ARGS__}

#define STELA_METHOD_NAME(MEMBER, NAME)                                         \
  stela::Method<&reflected_typedef::MEMBER>{#NAME}

#define STELA_METHOD(MEMBER)                                                    \
  STELA_METHOD_NAME(MEMBER, MEMBER)

#define STELA_METHOD_NAME_SIG(MEMBER, NAME, SIGNATURE)                          \
  stela::Method<stela::overload_mem<SIGNATURE>(&reflected_typedef::MEMBER)>{#NAME}

#define STELA_METHOD_SIG(MEMBER, SIGNATURE)                                     \
  STELA_METHOD_NAME_SIG(MEMBER, MEMBER, SIGNATURE)

#define STELA_TYPEALIAS(NAME, TYPE)                                             \
  stela::Typealias<Type>{NAME}

#define STELA_CONSTANT(NAME, VALUE)                                             \
  stela::Constant{NAME, VALUE}

#define STELA_ENUM_CASE(NAME, VALUE)                                            \
  stela::EnumConstant{#NAME, reflected_typedef::VALUE}

#define STELA_THIS reflected_typedef

}

#endif
