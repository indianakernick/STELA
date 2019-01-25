//
//  c standard library.cpp
//  STELA
//
//  Created by Indi Kernick on 4/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "c standard library.hpp"

#include "log output.hpp"
#include "scope manager.hpp"

using namespace stela;

namespace {

retain_ptr<ast::ExtFunc> makeUnary(
  const ast::BtnTypePtr &type,
  ast::Name name,
  const std::string &mangledName
) {
  auto func = make_retain<ast::ExtFunc>();
  func->name = name;
  func->mangledName = mangledName;
  func->params.push_back({ast::ParamRef::val, type});
  func->ret = type;
  return func;
}

retain_ptr<ast::ExtFunc> makeBinary(
  const ast::BtnTypePtr &type,
  ast::Name name,
  const std::string &mangledName
) {
  auto func = make_retain<ast::ExtFunc>();
  func->name = name;
  func->mangledName = mangledName;
  func->params.push_back({ast::ParamRef::val, type});
  func->params.push_back({ast::ParamRef::val, type});
  func->ret = type;
  return func;
}

}

AST stela::includeCmath(sym::Builtins &btn, LogSink &sink) {
  Log log{sink, LogCat::semantic};
  log.module("cmath");
  AST module;
  module.name = "cmath";
  
  module.global.push_back(makeUnary(btn.Real, "fabs", "fabsf"));
  module.global.push_back(makeBinary(btn.Real, "fmod", "fmodf"));
  
  module.global.push_back(makeUnary(btn.Real, "exp", "expf"));
  module.global.push_back(makeUnary(btn.Real, "exp2", "exp2f"));
  module.global.push_back(makeUnary(btn.Real, "expm1", "expm1f"));
  module.global.push_back(makeUnary(btn.Real, "log", "logf"));
  module.global.push_back(makeUnary(btn.Real, "log10", "log10f"));
  module.global.push_back(makeUnary(btn.Real, "log2", "log2f"));
  module.global.push_back(makeUnary(btn.Real, "log1p", "log1pf"));
  
  module.global.push_back(makeUnary(btn.Real, "pow", "powf"));
  module.global.push_back(makeUnary(btn.Real, "sqrt", "sqrtf"));
  module.global.push_back(makeUnary(btn.Real, "cbrt", "cbrtf"));
  module.global.push_back(makeUnary(btn.Real, "hypot", "hypotf"));
  
  module.global.push_back(makeUnary(btn.Real, "sin", "sinf"));
  module.global.push_back(makeUnary(btn.Real, "cos", "cosf"));
  module.global.push_back(makeUnary(btn.Real, "tan", "tanf"));
  module.global.push_back(makeUnary(btn.Real, "asin", "asinf"));
  module.global.push_back(makeUnary(btn.Real, "acos", "acosf"));
  module.global.push_back(makeUnary(btn.Real, "atan", "atanf"));
  module.global.push_back(makeBinary(btn.Real, "atan2", "atan2f"));
  
  module.global.push_back(makeUnary(btn.Real, "ceil", "ceilf"));
  module.global.push_back(makeUnary(btn.Real, "floor", "floorf"));
  module.global.push_back(makeUnary(btn.Real, "trunc", "truncf"));
  module.global.push_back(makeUnary(btn.Real, "round", "roundf"));
  
  return module;
}
