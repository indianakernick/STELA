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
#include "generation.hpp"

int main() {
  int failures = 0;
  failures += testFormat();
  failures += testLexer();
  failures += testSyntax();
  failures += testSemantics();
  failures += testGeneration();
  if (failures == 0) {
    std::cout << "ALL PASSED!\n";
  } else if (failures == 1) {
    std::cout << "1 TEST FAILED!\n";
  } else {
    std::cout << failures << " TESTS FAILED!\n";
  }
  if (failures > 255) {
    return 255;
  } else {
    return failures;
  }
}
