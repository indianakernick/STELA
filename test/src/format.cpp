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
#include <STELA/console format.hpp>

TEST_GROUP(Format, {
  const char *source = R"(
type Rational struct {
  n: Int64;
  d: Int64;
};

type Vec3 struct {
  x: Float;
  y: Float;
  z: Float;
};

let origin = Vec3(0, 0, 0);

type IntStack struct {
  data: [Int];
};

func (self: inout IntStack) push(value: Int) {
  self.data.push_back(value);
}
func (self: inout IntStack) pop() {
  let top = data.back();
  data.pop_back();
  return top;
}
func (self: IntStack) top() {
  return data.back();
}
func (self: IntStack) empty() {
  return data.empty();
}
func justPop(stack: inout IntStack) {
  while (!stack.empty()) {
    stack.pop();
    continue; // for no reason
  }
}

let less = func(a: Int, b: Int) -> Bool {
  return a < b;
};

func isEqual(a: Int, b: Int) -> Bool {
  return a == b;
}

let equal = isEqual;

let ray = ['r', 'a', 'y'];

func sort(array: [Int], pred: func(Int, Int)->Bool) {
  // sorting...
}

func customSwap(a: [Int], b: [Int], swap: func(inout Int, inout Int)) {
  // yeah...
}

type EmptyStruct struct {};

func ifs() {
  switch (1) {}

  if (expr) {
    var dummy: Char = (5 ? 'e' : 'f');
  }

  if (expr) {
    var dummy = expr;
  } else {
    var dummy = expr;
  }

  if (expr) {
    var dummy = expr;
  } else if (expr) {
    var dummy = expr;
  } else {
    var dummy = expr;
  }
  
  {
    let blocks_are_statements = 5;
    let just_like_C = "yeah"[1];
  }
  
  type StrInt = func(String) -> Int;
  
  while (expr) {
    thing++;
    thing += ~yeah;
  }

  for (i := 0; i != 10; i++) {
    thing++;
    thing += ~yeah;
  }
  for (i = 0; i != 10; i++) {
    thing++;
    thing += ~yeah;
  }
  for (; false && true; ) {
    thing++;
    thing += ~yeah;
  }
}

type Dir Int;
let Dir_up: Dir = 0;
let Dir_right: Dir = 1;
let Dir_down: Dir = 2;
let Dir_left: Dir = 3;

func anotherDummy(dir: Dir) {
  value := 5;
  switch (dir) {
    case (Dir_up) {
      value += 2;
      continue;
    }
    case (Dir_right) {
      value <<= 2;
      break;
    }
    case (Dir_down) {
      value--;
      return value & 3;
    }
    case (Dir_left) return 5;
    default value -= 1;
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
