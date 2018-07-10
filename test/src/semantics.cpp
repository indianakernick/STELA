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
  
  TEST(Declarations, {
    const char *source = R"(
      func myFunction() {
        
      }
    
      struct MyStruct {
      
      }
    
      enum MyEnum {
      
      }
    
      let myGlobalConstant = 8;
    
      var myGlobalVariable;
    )";
    const auto [sym, ast] = createSym(source, log);
    ASSERT_EQ(ast.global.size(), 5);
    ASSERT_EQ(sym.scopes.size(), 1);
  });
});
