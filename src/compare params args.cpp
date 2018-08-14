//
//  compare params args.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "compare params args.hpp"

using namespace stela;

namespace {

template <typename Pred>
bool compare(
  const sym::FuncParams &params,
  const sym::FuncParams &args,
  Pred pred
) {
  return params.size() == args.size()
    && std::equal(params.begin(), params.end(), args.begin(), pred);
}

}

bool stela::compatParams(
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  return compare(params, args, [] (sym::ExprType param, sym::ExprType arg) {
    return param.type == arg.type && sym::callMutRef(param, arg);
  });
}

bool stela::sameParams(
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  return compare(params, args, [] (sym::ExprType param, sym::ExprType arg) {
    return param.type == arg.type;
  });
}
