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
#include "semantics.hpp"

int main() {
  const int failures = !testLexer() + !testSyntax() + !testSemantics();
  if (failures == 0) {
    std::cout << "ALL PASSED!\n";
  } else {
    std::cout << "SOME TESTS FAILED!\n";
  }
  return failures;
}
