//
//  compare params args.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "compare params args.hpp"

#include "compare types.hpp"

using namespace stela;

namespace {

template <typename Pred>
bool compare(
  const sym::FuncParams &params,
  const sym::FuncParams &args,
  Pred pred
) {
  return params.size() == args.size()
    && std::equal(params.cbegin(), params.cend(), args.cbegin(), pred);
}

}

bool stela::compatParams(
  const NameLookup &lkp,
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  return compare(params, args, [&lkp] (sym::ExprType param, sym::ExprType arg) {
    return compareTypes(lkp, param.type, arg.type) && sym::callMutRef(param, arg);
  });
}

bool stela::sameParams(
  const NameLookup &lkp,
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  return compare(params, args, [&lkp] (sym::ExprType param, sym::ExprType arg) {
    return compareTypes(lkp, param.type, arg.type);
  });
}
