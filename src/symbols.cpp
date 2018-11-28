//
//  symbols.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "symbols.hpp"

using namespace stela;

sym::Symbol::~Symbol() = default;
sym::TypeAlias::~TypeAlias() = default;
sym::Object::~Object() = default;
sym::Func::~Func() = default;
sym::Lambda::~Lambda() = default;
sym::BtnFunc::~BtnFunc() = default;

const sym::ExprType sym::null_type {};
const sym::ExprType sym::void_type {
  make_retain<ast::BtnType>(ast::BtnTypeEnum::Void), {}, {}
};
