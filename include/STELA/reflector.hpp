//
//  reflector.hpp
//  STELA
//
//  Created by Indi Kernick on 29/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_reflector_hpp
#define stela_reflector_hpp

#include "ast.hpp"
#include "meta.hpp"
#include "wrap function.hpp"

namespace stela {

std::string mangledName(std::string_view);
std::string mangledName(const std::string &);

template <typename Type, typename = void>
struct reflect {
  static_assert(sizeof(Type) == 0, "Type is not reflectible");
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

template <typename Signature, bool Method>
struct write_sig;

} // namespace detail

class Reflector {
  template <typename Type>
  static inline const size_t typeID = TypeID<struct ReflectorFamily>::id<Type>;

public:
  template <typename Type>
  void reflectType() {
    const size_t index = typeID<Type>;
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
    constant->expr = std::move(init);
    reflectDecl(constant);
  }
  
  void reflectDecl(ast::DeclPtr decl) {
    decls.push_back(decl);
  }

  template <typename Type>
  ast::TypePtr getType() {
    const size_t index = typeID<Type>;
    reflectType<Type>();
    return types[index];
  }
  
  void appendDeclsTo(std::vector<ast::DeclPtr> &otherDecls) {
    otherDecls.insert(otherDecls.end(), decls.cbegin(), decls.cend());
  }
  
private:
  std::vector<ast::TypePtr> types;
  std::vector<ast::DeclPtr> decls;
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
  template <typename Func>
  static void write(Func &func, Reflector &refl) {
    func.receiver = getParamType<Class>(refl);
    write_sig<Ret(*)(Params...) noexcept(Noexcept), false>::write(func, refl);
  }
};

template <bool Noexcept, typename Ret, typename... Params>
struct write_sig<Ret(*)(Params...) noexcept(Noexcept), false> {
  template <typename Func>
  static void write(Func &func, Reflector &refl) {
    static_assert(!std::is_reference_v<Ret>, "Stela functions cannot return references");
    func.ret = refl.getType<Ret>();
    func.params.reserve(sizeof...(Params));
    (func.params.push_back(getParamType<Params>(refl)), ...);
  }
};

} // namespace detail

}

#endif
