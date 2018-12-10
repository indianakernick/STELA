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
#include "gen context.hpp"

namespace llvm {

class Type;
class FunctionType;
class PointerType;
class StructType;

}

namespace stela {

llvm::Type *generateType(gen::Ctx, ast::Type *);
llvm::FunctionType *generateFuncSig(gen::Ctx, const ast::FuncType &);
gen::String generateFuncName(gen::Ctx, const ast::FuncType &);
llvm::PointerType *getVoidPtr(gen::Ctx);
llvm::PointerType *getCloDataPtr(gen::Ctx);

ast::Type *concreteType(ast::Type *);

template <typename Concrete>
Concrete *concreteType(ast::Type *type) {
  return dynamic_cast<Concrete *>(concreteType(type));
}

llvm::StructType *generateLambdaCapture(gen::Ctx, const ast::Lambda &);

}

#endif
