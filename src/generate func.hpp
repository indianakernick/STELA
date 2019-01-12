//
//  generate func.hpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_func_hpp
#define stela_generate_func_hpp

// @TODO remove this header

#include "ast.hpp"
#include "symbols.hpp"
#include "gen context.hpp"

namespace llvm {

class Type;
class Function;
class Module;
class LLVMContext;
class IntegerType;
class PointerType;

}

namespace stela {

std::string generateNullFunc(gen::Ctx, const ast::FuncType &);
std::string generateMakeFunc(gen::Ctx, ast::FuncType &);
std::string generateLambda(gen::Ctx, const ast::Lambda &);
std::string generateMakeLam(gen::Ctx, const ast::Lambda &);

}

#endif
