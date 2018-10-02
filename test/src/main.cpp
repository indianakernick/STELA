//
//  main.cpp
//  Test
//
//  Created by Indi Kernick on 17/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "macros.hpp"

#include "lexer.hpp"
#include "syntax.hpp"
#include "format.hpp"
#include "semantics.hpp"

int main() {
  const int failures = /*testFormat() +*/ testLexer() + testSyntax() + testSemantics();
  if (failures == 0) {
    std::cout << "ALL PASSED!\n";
  } else if (failures == 1) {
    std::cout << "1 TEST FAILED!\n";
  } else {
    std::cout << failures << " TESTS FAILED!\n";
  }
  return failures;
}
