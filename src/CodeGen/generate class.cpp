//
//  generate class.cpp
//  STELA
//
//  Created by Indi Kernick on 28/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "inst data.hpp"
#include "gen helpers.hpp"
#include "generate type.hpp"
#include <llvm/Support/DynamicLibrary.h>

using namespace stela;

namespace {

void addSymbol(const ast::UserCtor &ctor) {
  llvm::sys::DynamicLibrary::AddSymbol(
    ctor.name,
    reinterpret_cast<void *>(ctor.addr)
  );
}

}

template <>
llvm::Function *stela::genFn<PFGI::usr_dtor>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, unaryCtorFor(type), usr->dtor.name);
  assignUnaryCtorAttrs(func);
  addSymbol(usr->dtor);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::usr_def_ctor>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, unaryCtorFor(type), usr->defCtor.name);
  assignUnaryCtorAttrs(func);
  addSymbol(usr->defCtor);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::usr_cop_ctor>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, binaryCtorFor(type), usr->copCtor.name);
  assignBinaryCtorAttrs(func);
  addSymbol(usr->copCtor);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::usr_cop_asgn>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, binaryCtorFor(type), usr->copAsgn.name);
  assignBinaryAliasCtorAttrs(func);
  addSymbol(usr->copAsgn);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::usr_mov_ctor>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, binaryCtorFor(type), usr->movCtor.name);
  assignBinaryCtorAttrs(func);
  addSymbol(usr->movCtor);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::usr_mov_asgn>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, binaryCtorFor(type), usr->movAsgn.name);
  assignBinaryCtorAttrs(func);
  addSymbol(usr->movAsgn);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::usr_eq>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, compareFor(type), usr->eq.name);
  assignCompareAttrs(func);
  addSymbol(usr->eq);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::usr_lt>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, compareFor(type), usr->lt.name);
  assignCompareAttrs(func);
  addSymbol(usr->lt);
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::usr_bool>(InstData data, ast::UserType *usr) {
  llvm::Type *type = generateType(data.mod->getContext(), usr);
  llvm::Function *func = declareCFunc(data.mod, boolFor(type), usr->boolConv.name);
  assignBoolAttrs(func);
  addSymbol(usr->boolConv);
  return func;
}
