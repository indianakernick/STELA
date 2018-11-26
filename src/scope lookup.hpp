//
//  scope lookup.hpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_lookup_hpp
#define stela_scope_lookup_hpp

#include "ast.hpp"
#include "context.hpp"

namespace stela {

sym::Symbol *find(sym::Scope *, ast::Name);
sym::Scope *getModuleScope(sym::Ctx, ast::Name, Loc);

ast::TypeAlias *lookupTypeName(sym::Ctx, ast::NamedType &);
ast::TypePtr lookupStrongType(sym::Ctx, const ast::TypePtr &);
ast::TypePtr lookupConcreteType(sym::Ctx, const ast::TypePtr &);

template <typename Type>
retain_ptr<Type> lookupConcrete(const sym::Ctx ctx, const ast::TypePtr &type) {
  return dynamic_pointer_cast<Type>(lookupConcreteType(ctx, type));
}

ast::TypePtr getFuncType(Log &, ast::Func &, Loc);
ast::TypePtr getLambdaType(sym::Ctx, ast::Lambda &);

}

#endif
