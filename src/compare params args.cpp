//
//  compare params args.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "compare params args.hpp"

#include "algorithms.hpp"
#include "compare types.hpp"

using namespace stela;

bool stela::compatParams(
  sym::Ctx ctx,
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  return equal_size(params, args, [ctx] (const auto &param, const auto &arg) {
    return compareTypes(ctx, param.type, arg.type) && sym::callMutRef(param, arg);
  });
}

bool stela::sameParams(
  sym::Ctx ctx,
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  return equal_size(params, args, [ctx] (const auto &param, const auto &arg) {
    return compareTypes(ctx, param.type, arg.type);
  });
}
