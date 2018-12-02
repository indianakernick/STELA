//
//  generate func.cpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate func.hpp"

#include "unreachable.hpp"
#include "generate type.hpp"

using namespace stela;

gen::String stela::generateNullFunc(gen::Ctx ctx, const ast::FuncType &type) {
  gen::String name;
  name += "f_null_";
  // @TODO maybe consider caching type names
  // generateFuncName does that same work that generateNullFunc does
  name += generateFuncName(ctx, type);
  if (ctx.inst.funcNotInst(name)) {
    ctx.func += "[[noreturn]] static ";
    ctx.func += generateTypeOrVoid(ctx, type.ret.get());
    ctx.func += ' ';
    ctx.func += name;
    ctx.func += "(void *";
    for (const ast::ParamType &param : type.params) {
      ctx.func += ", ";
      ctx.func += generateType(ctx, param.type.get());
      if (param.ref == ast::ParamRef::inout) {
        ctx.func += " &";
      }
    }
    ctx.func += ") noexcept {\n";
    ctx.func += "panic(\"Calling null function pointer\");\n";
    ctx.func += "}\n";
  }
  return name;
}

gen::String stela::generateMakeFunc(gen::Ctx ctx, ast::FuncType &type) {
  gen::String name;
  name += "f_makefunc_";
  name += generateFuncName(ctx, type);
  if (ctx.inst.funcNotInst(name)) {
    ctx.func += "static ";
    ctx.func += generateType(ctx, &type);
    ctx.func += " ";
    ctx.func += name;
    ctx.func += "(const ";
    ctx.func += generateFuncSig(ctx, type);
    ctx.func += R"( func) noexcept {
auto *ptr = static_cast<FuncClosureData *>(allocate(sizeof(FuncClosureData)));
new (ptr) FuncClosureData(); // setup virtual destructor
ptr->count = 1;
return {func, ClosureDataPtr{static_cast<ClosureData *>(ptr)}};
}
)";
  }
  return name;
}

gen::String stela::boolConv(gen::Ctx ctx, const sym::ExprType &etype, const ast::Name name) {
  if (auto *func = dynamic_cast<ast::FuncType *>(etype.type.get())) {
    gen::String str;
    str += name;
    if (etype.ref == sym::ValueRef::ref) {
      str += "->";
    } else {
      str += ".";
    }
    str += "func != &";
    str += generateNullFunc(ctx, *func);
    return str;
  }
  UNREACHABLE();
}
