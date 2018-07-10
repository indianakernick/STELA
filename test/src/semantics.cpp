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
    const auto [symbols, ast] = createSym("", log);
    ASSERT_EQ(symbols.scopes.size(), 1);
    ASSERT_TRUE(ast.global.empty());
  });
  
  TEST(Redefinition of function, {
    const char *source = R"(
      func myFunction() -> Void {
        
      }
    
      func myFunction() -> Void {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
});
