//
//  format.cpp
//  Test
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "format.hpp"

#include <fstream>
#include "macros.hpp"
#include <STELA/html format.hpp>
#include <STELA/plain format.hpp>
#include <STELA/console format.hpp>
#include <STELA/syntax analysis.hpp>

TEST_GROUP(Format, {
  const char *source = R"(
type Rational = struct {
  n: sint;
  d: sint;
};

type Vec3 struct {
  x: real;
  y: real;
  z: real;
};

let origin = make Vec3 {0.0, 0.0, 0.0};
let nesting = make sint make real make uint {};

type IntStack [sint];

func (self: inout IntStack) push(value: sint) {
  push_back(self, value);
}
func (self: inout IntStack) pop() {
  pop_back(self);
}
func (self: inout IntStack) pop_top() -> sint {
  let top = self[size(self) - 1u];
  pop_back(self);
  return top;
}
func (self: IntStack) top() -> sint {
  return self[size(self) - 1u];
}
func (self: IntStack) empty() -> bool {
  return size(self) == 0u;
}
func justPop(stack: inout IntStack) {
  while (!stack.empty()) {
    stack.pop();
    continue; // for no reason
  }
}

let less = func(a: sint, b: sint) -> bool {
  return a < b;
};

func isEqual(a: sint, b: sint) -> bool {
  return a == b;
}

let equal = isEqual;

let ray = ['r', 'a', 'y'];

func sort(array: [sint], pred: func(sint, sint)->bool) {
  // sorting...
}

func customSwap(a: [sint], b: [sint], swap: func(inout sint, inout sint)) {
  // yeah...
}

type EmptyStruct struct {};

func ifs() {
  switch (1) {}

  if (false) {
    var dummy: char = (true ? 'e' : 'f');
  }

  if (true) {
    var dummy = 0;
  } else {
    var dummy = 1;
  }

  if (false) {
    var dummy = 2;
  } else if (true) {
    var dummy = 3;
  } else {
    var dummy = 4;
  }
  
  {
    let blocks_are_statements = 5;
    let just_like_C = "yeah"[1];
  }
  
  type StrInt = func([char]) -> sint;
  
  var thing = 0u;
  let yeah = 99u;
  while (false) {
    thing++;
    thing += ~yeah;
  }
  
  thing = 11u;

  for (i := 0; i != 10; i++) {
    thing++;
    thing += ~yeah;
  }
  for (; false && true; ) {
    thing++;
    thing += ~yeah;
  }
}

type Dir sint;
let Dir_up    = make Dir 0;
let Dir_right = make Dir 1;
let Dir_down  = make Dir 2;
let Dir_left  = make Dir 3;

func anotherDummy(dir: Dir) -> uint {
  value := 5u;
  switch (dir) {
    case (Dir_up) {
      value += 2u;
      continue;
    }
    case (Dir_right) {
      value <<= 2u;
      break;
    }
    case (Dir_down) {
      value--;
      return value & 3u;
    }
    case (Dir_left) return 5u;
    default value -= 1u;
  }
  return value;
}
)";

  stela::StreamLog log;
  stela::fmt::Tokens tokens;
  
  TEST(Get tokens, {
    tokens = stela::format(source, log);
    ASSERT_FALSE(tokens.empty());
  });
  
  TEST(Plain string, {
    std::string str = stela::plainFormat(tokens, 4);
    ASSERT_FALSE(str.empty());
    std::cout << str;
  });
  
  TEST(Plain stream first node, {
    const stela::AST ast = stela::createAST(source, log);
    stela::fmt::Tokens nodeTokens = stela::format(ast.global.front().get());
    stela::plainFormat(std::cout, nodeTokens, 4);
    std::cout << '\n';
  });
  
  TEST(Console, {
    stela::conFormat(tokens);
  });
  
  TEST(HTML default styles, {
    stela::HTMLstyles styles;
    styles.doc = true;
    styles.def = true;
    const std::string html = stela::htmlFormat(tokens, styles);
    ASSERT_FALSE(html.empty());
    std::ofstream file("default.html", std::ios::ate);
    ASSERT_TRUE(file.is_open());
    file << html;
  });
  
  TEST(HTML non-default styles, {
    stela::HTMLstyles styles;
    styles.doc = true;
    styles.def = false;
    const std::string html = stela::htmlFormat(tokens, styles);
    ASSERT_FALSE(html.empty());
    std::ofstream file("not default.html", std::ios::ate);
    ASSERT_TRUE(file.is_open());
    file << html;
  });
})
