//
//  generate closure.hpp
//  STELA
//
//  Created by Indi Kernick on 15/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_closure_hpp
#define stela_generate_closure_hpp

#include "gen context.hpp"

namespace llvm {

class Function;
class Module;
class Value;

}

namespace stela {

namespace ast {

struct Lambda;

}

class FuncBuilder;

llvm::Function *genLambdaBody(gen::Ctx, ast::Lambda &);
llvm::Value *closureCapAddr(FuncBuilder &, llvm::Value *, uint32_t);
llvm::Value *closureFun(FuncBuilder &, llvm::Value *);
llvm::Value *closureDat(FuncBuilder &, llvm::Value *);
llvm::Function *getVirtualDtor(gen::Ctx, const ast::Lambda &);
void setDtor(FuncBuilder &, llvm::Value *, llvm::Function *);

}

#endif
