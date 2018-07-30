//
//  compare params args.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "compare params args.hpp"

/// Argument types are convertible to parameter types
bool stela::convParams(
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  if (params.size() != args.size()) {
    return false;
  }
  for (size_t i = 0; i != params.size(); ++i) {
    if (convertibleTo(args[i].cat, params[i].cat)) {
      if (params[i].cat == sym::ValueCat::rvalue) {
        // @TODO lookup type conversions
     // if (args[i].type is not convertible to params[i].type) {
        if (params[i].type != args[i].type) {
          return false;
        }
      } else if (params[i].type != args[i].type) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

/// Argument types are compatible with parameter types. (Checks ValueCat)
bool stela::compatParams(
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  if (params.size() != args.size()) {
    return false;
  }
  for (size_t i = 0; i != params.size(); ++i) {
    if (!convertibleTo(args[i].cat, params[i].cat) || params[i].type != args[i].type) {
      return false;
    }
  }
  return true;
}

/// Argument types are the same as parameter types. ValueCat may be different
bool stela::sameParams(
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  if (params.size() != args.size()) {
    return false;
  }
  for (size_t i = 0; i != params.size(); ++i) {
    if (params[i].type != args[i].type) {
      return false;
    }
  }
  return true;
}
