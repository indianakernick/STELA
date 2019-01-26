//
//  generate type.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_type_hpp
#define stela_generate_type_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "gen context.hpp"
#include "Utils/assert down cast.hpp"

namespace llvm {

class Type;
class FunctionType;
class PointerType;
class StructType;
class LLVMContext;

}

namespace stela {

// @TODO Use Signature in more places
struct Signature {
  ast::ParamType receiver;
  ast::ParamTypes params;
  ast::TypePtr ret;
};

llvm::Type *generateType(llvm::LLVMContext &, ast::Type *);
llvm::FunctionType *generateSig(llvm::LLVMContext &, const Signature &);
// @TODO this might not be necessary if we remove void * from regular functions
llvm::FunctionType *generateSig(llvm::LLVMContext &, const ast::ExtFunc &);

Signature getSignature(const ast::Func &);
Signature getSignature(const ast::Lambda &);
Signature getSignature(const ast::FuncType &);

void assignAttrs(llvm::Function *, const Signature &);
llvm::Type *convertParam(llvm::LLVMContext &, const ast::ParamType &);
std::string generateFuncName(gen::Ctx, const ast::FuncType &);

ast::Type *concreteType(ast::Type *);
bool isBoolType(ast::Type *);
bool isBoolCast(ast::Type *, ast::Type *);

template <typename Concrete>
Concrete *concreteType(ast::Type *type) {
  return dynamic_cast<Concrete *>(concreteType(type));
}

template <typename Concrete>
Concrete *assertConcreteType(ast::Type *type) {
  return assertDownCast<Concrete>(concreteType(type));
}

llvm::StructType *generateLambdaCapture(llvm::LLVMContext &, const ast::Lambda &);

}

#endif
