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
struct Rational {
  // all members of a struct are implicitly public
  var n: Int64;
  var d: Int64;
  
  init(num: Int64) {
    // self is optional
    // self.n = num;
    // self.d = 1;
    n = num;
    d = 1;
  }
  
  // if we didn't provide any constructors then this one would have been
  // implicitly generated. Since we provided our own constructor,
  // init(num: Int64)
  // this one is not implicitly generated, so we have to define it
  init(n: Int64, d: Int64) {
    self.n = n;
    self.d = d;
  }
} // optional ;

// global variables are not allowed
// var half = Rational(1, 2);
// global constants are allowed though
let half = Rational(1, 2);
// speaking of global, like C++ but unlike Swift,
// constants, type declartions and functions are allowed at global scope
// but nothing else

struct Vec3 {
  // if all properties have default values, there is an implicit init()
  // but only if the user doesn't declare any other constructors
  var x = 0.0;
  var y = 0.0;
  var z = 0.0;
  
  /*
  This declaration prevents the init(Float, Float, Float) constructor from being
  implicitly generated
  init(val: Float) {
    x = val;
    y = val;
    z = val;
  }
  
  not implicity generated if the above constructor is present
  init(x: Float, y: Float, z: Float) {
    self.x = x;
    self.y = y;
    self.z = z;
  }
  */
}

// calling implicitly generated constructor
let origin = Vec3(0, 0, 0);

struct IntStack {
  // member variables are implicitly private
  // you can mark them private if you want
  private var data: [Int];
  /*
  the `data` member is an array of ints
  arrays are objects with member functions
  arrays have very similar interfaces to std::vector<> from C++
  */
  
   init() {} // implicitly generated
  
  // member functions are implicitly public
  // you can mark them public if you want
  public func push(value: Int) {
    data.push_back(value);
  }
  func pop() {
    let top = data.back();
    data.pop_back();
    return top;
  }
  func top() {
    return data.back();
  }
  func empty() {
    return data.empty();
  }
  
  static func justPop(stack: inout IntStack) {
    while (!stack.empty()) {
      stack.pop();
      continue; // for no reason
    }
  }
};

enum ABC {
  a = 65,
  b = 66,
  c = 67
}

let map: {String: [Int]} = {
  "one_two_three": [1, 2, 3],
  "four_five_six": [4, 5, 6],
  "seven": [7],
  "eight_nine": [8, 9]
};

let empty = {};

let less = lambda (a: Int, b: Int) {
  return a < b;
};

func isEqual(a: Int, b: Int) {
  return a == b;
}

let equal = isEqual;

func sort(array: [Int], pred: (Int, Int) -> Bool) -> Void {
  // sorting...
}

func customSwap(a: [Int], b: [Int], swap: (inout Int, inout Int) -> Void) {
  // yeah...
}

struct EmptyStruct {}
struct EmptyEnum {}

func ifs() {
  switch (1) {}

  if (expr) {
    var dummy = (5 ? 'e' : 'f');
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
  
  if (true);
  
  typealias StrInt = {String:Int};
  
  while (expr) {
    ++thing;
    thing += ~yeah;
  }
  
  repeat {
    ++thing;
    thing += ~yeah;
  } while (expr);

  for (var i = 0; i != 10; ++i) {
    ++thing;
    thing += ~yeah;
  }
  for (i = 0; i != 10; ++i) {
    ++thing;
    thing += ~yeah;
  }
  for (; false; ) {
    ++thing;
    thing += ~yeah;
  }
}

enum Dir {
  up, right, down, left
}

func anotherDummy(dir: Dir) {
  var value = 5;
  switch (dir) {
    case Dir.up:
      value += 2;
      fallthrough;
    case Dir.right:
      value <<= 2;
      break;
    case Dir.down:
      ++value;
      return value & 3;
    case Dir.left:
      return 5;
    default:
      value--;
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
    if (file.is_open()) {
      file << html;
    }
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
