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
#include "assert down cast.hpp"

namespace llvm {

class Type;
class FunctionType;
class PointerType;
class StructType;
class LLVMContext;

}

namespace stela {

llvm::Type *generateType(llvm::LLVMContext &, ast::Type *);
llvm::FunctionType *generateFuncSig(llvm::LLVMContext &, const ast::Func &);
llvm::FunctionType *generateLambSig(llvm::LLVMContext &, const ast::FuncType &);

void assignAttributes(llvm::Function *, const sym::FuncParams &, ast::Type *);
llvm::Type *convertParam(llvm::LLVMContext &, const ast::ParamType &);
std::string generateFuncName(gen::Ctx, const ast::FuncType &);

ast::Type *concreteType(ast::Type *);

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
