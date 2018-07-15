//
//  format.cpp
//  Test
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "format.hpp"

#include "console format.hpp"

bool testFormat() {
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
let half = make Rational(1, 2);
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
let origin = make Vec3(0, 0, 0);

struct IntStack {
  // member variables are implicitly private
  // you can mark them private if you want
  private var data: [Int];
  /*
  the `data` member is an array of ints
  arrays are objects with member functions
  arrays have very similar interfaces to std::vector<> from C++
  */
  
  // init() {}  implicitly generated
  
  // member functions are implicitly public
  // you can mark them public if you want
  func push(value: Int) {
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
};

)";

  stela::StreamLog log;
  stela::fmt::Tokens tokens = stela::format(source, log);
  stela::conFormat(tokens);
  
  return true;
}
