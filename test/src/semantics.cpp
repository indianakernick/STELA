//
//  semantics.cpp
//  Test
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "semantics.hpp"

#include "macros.hpp"
#include <STELA/semantic analysis.hpp>

using namespace stela;
using namespace stela::ast;

TEST_GROUP(Semantics, {
  StreamLog log;
  
  TEST(Empty source, {
    const auto [symbols, ast] = annotateAST("", log);
    ASSERT_TRUE(symbols.global.empty());
    ASSERT_TRUE(ast.global.empty());
  });
});
