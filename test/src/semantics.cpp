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
  
  TEST(Return type inference, {
    const char *source = R"(
      func returnInt() {
        return 5;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Redef func, {
    const char *source = R"(
      func myFunction() -> Void {
        
      }
      func myFunction() -> Void {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Redef func with params, {
    const char *source = R"(
      func myFunction(i: Int, f: Float) -> Void {
        
      }
      func myFunction(i: Int, f: Float) -> Void {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Redef func alias, {
    const char *source = R"(
      typealias Number = Int;
      func myFunction(i: Number) -> Void {
        
      }
      func myFunction(i: Int) -> Void {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Redef func multi alias, {
    const char *source = R"(
      typealias Integer = Int;
      typealias Number = Integer;
      typealias SpecialNumber = Number;
      typealias TheNumber = SpecialNumber;
      func myFunction(i: TheNumber) -> Void {
        
      }
      func myFunction(i: Int) -> Void {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });

  TEST(Undefined symbol, {
    const char *source = R"(
      func myFunction(i: Number) -> Void {
        
      }
      func myFunction(i: Int) -> Void {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });

  TEST(Function overloading, {
    const char *source = R"(
      func myFunction(n: Float) -> Void {
        
      }
      func myFunction(n: Int) -> Void {
        
      }
    )";
    createSym(source, log);
  });
});
