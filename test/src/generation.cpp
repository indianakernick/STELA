//
//  generation.cpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generation.hpp"

#include <fstream>
#include <iostream>
#include "macros.hpp"
#include <STELA/code generation.hpp>
#include <STELA/syntax analysis.hpp>
#include <STELA/semantic analysis.hpp>

using namespace stela;

namespace {

Symbols createSym(const std::string_view source, LogSink &log) {
  AST ast = createAST(source, log);
  Symbols syms = initModules(log);
  compileModule(syms, ast, log);
  return syms;
}

std::string generateCpp(const std::string_view source, LogSink &log) {
  return generateCpp(createSym(source, log), log);
}

void printSource(const std::string &cpp) {
  const std::string endStr = "END BUILTIN LIBRARY";
  const size_t btnEnd = cpp.rfind(endStr) + endStr.size();
  size_t srcBeg = btnEnd;
  while (std::isspace(cpp[srcBeg])) {
    ++srcBeg;
  }
  std::cout << '\n';
  std::cout << std::string_view{cpp.data() + srcBeg, cpp.size() - srcBeg} << '\n';
  std::cout << '\n';
}

bool validCode(const std::string &cpp) {
  const std::string cppFileName = "temporary_source.cpp";
  const std::string errFileName = "temporary_error.txt";
  const std::string exeFileName = "temporary_executable";
  printSource(cpp);
  std::ofstream cppFile(cppFileName);
  if (!cppFile.is_open()) {
    std::cout << "Could not open temporary source file\n";
    return false;
  }
  cppFile << cpp;
  cppFile.close();
  std::string command = "$CXX -std=c++17 ";
  command.append(cppFileName);
  command.append(" -o ");
  command.append(exeFileName);
  command.append(" 2> ");
  command.append(errFileName);
  system(command.c_str());
  std::ifstream errFile(errFileName);
  if (!errFile.is_open()) {
    std::cout << "Could not open temporary error file\n";
    return false;
  }
  errFile.seekg(0, std::ios::end);
  std::string errMsg(static_cast<size_t>(errFile.tellg()), '\0');
  errFile.seekg(0, std::ios::beg);
  errFile.read(errMsg.data(), static_cast<std::streamsize>(errMsg.size()));
  if (errMsg.size() > 1) {
    std::cout << errMsg << '\n';
    return false;
  }
  return true;
}

bool generate(const std::string_view source, LogSink &log) {
  return validCode(generateCpp(source, log));
}

}

#define ASSERT_COMPILES(SOURCE)                                                 \
  const bool success = generate(SOURCE, log);                                   \
  ASSERT_TRUE(success)
#define ASSERT_STELA_FAILS(SOURCE)                                              \
  ASSERT_THROWS(generate(SOURCE, log), FatalError)
#define ASSERT_CPP_FAILS(SOURCE)                                                \
  const bool success = generate(SOURCE, log);                                   \
  ASSERT_FALSE(success)

TEST_GROUP(Generation, {
  StreamSink stream;
  FilterSink log{stream, LogPri::status};
  
  /*
  
  TEST(Empty source, {
    ASSERT_COMPILES("");
  });
  
  TEST(Functions, {
    ASSERT_COMPILES(R"(
      func first() {}
      
      func second() -> sint {
        return 1;
      }
      
      func third(a: real) {}
      
      func fourth(a: bool, b: uint) {}
      
      func fifth(a: real, b: ref byte, c: sint) {}
      
      func (self: ref [struct {v: [sint];}]) sixth(a: ref sint) {}
    )");
  });
  
  TEST(Identical structs, {
    ASSERT_COMPILES(R"(
      func first(param: struct {value: sint;}) {}
      type Number = sint;
      func second(param: struct {value: Number;}) {}
    )");
  });
  
  TEST(Vars, {
    ASSERT_COMPILES(R"(
      func fn() {
        let a = 0;
        var b = 0.0;
      }
    )");
  });
  
  TEST(Expressions, {
    ASSERT_COMPILES(R"(
      func fac(n: sint) -> sint {
        return n == 0 ? 1 : n * fac(n - 1);
      }
    )");
  });
  
  TEST(If, {
    ASSERT_COMPILES(R"(
      func test(p: bool) -> real {
        if (!p && true) {
          return 11.0;
        } else {
          return -make real 1;
        }
      }
    )");
  });
  
  TEST(Member functions, {
    ASSERT_COMPILES(R"(
      type Vec2 struct {
        x: real;
        y: real;
      };
      
      let zero = make Vec2 {0.0, 0.0};
      let alsoZero = make Vec2 {};
      
      func (self: Vec2) mag2() -> real {
        return self.x * self.x + self.y * self.y;
      }
      
      func add(a: Vec2, b: Vec2) -> Vec2 {
        return make Vec2 {a.x + b.x, a.y + b.y};
      }
      
      func test() {
        let zero = alsoZero.mag2();
        let one_two = make Vec2 {1.0, 2.0};
        let three_four = make Vec2 {3.0, 4.0};
        let four_six = add(one_two, three_four);
        let five = one_two.mag2();
      }
    )");
  });
  
  TEST(Arrays, {
    ASSERT_COMPILES(R"(
      func squares(count: uint) -> [uint] {
        var array: [uint] = [];
        if (count == 0u) {
          return array;
        }
        reserve(array, count);
        for (i := 1u; i <= count; i++) {
          push_back(array, i * i);
        }
        return array;
      }

      func test() {
        var empty = squares(0u);
        let one_four_nine = squares(3u);
        empty = [];
        
        let nested: [[[real]]] = [[[0.0]]];
        let zero: real = nested[0][0][0];
      }
    )");
  });
  
  TEST(Strings, {
    ASSERT_COMPILES(R"(
      func test() {
        var hello = "Hello world";
        let w = hello[6];
        hello[0] = 'Y';
      }
    )");
  });
  
  TEST(Switch, {
    ASSERT_COMPILES(R"(
      type Dir uint;
      let Dir_up = make Dir 0;
      let Dir_right = make Dir 1;
      let Dir_down = make Dir 2;
      let Dir_left = make Dir 3;
      
      func toString(dir: Dir) -> [char] {
        switch (dir) {
          case (Dir_up) return "up";
          case (Dir_right) return "right";
          case (Dir_down) return "down";
          case (Dir_left) return "left";
          default return "";
        }
      }
      
      func weirdToString(dir: Dir) -> [char] {
        switch (dir) {
          case (Dir_up) continue;
          case (Dir_right) return "up or right";
          case (Dir_down) continue;
          case (Dir_left) return "down or left";
          default return "";
        }
      }
      
      func redundantBreaks(dir: Dir) -> [char] {
        var ret: [char];
        switch (dir) {
          case (Dir_up) {
            ret = "up";
            break;
          }
          case (Dir_right) {
            ret = "right";
            break;
          }
          case (Dir_down) {
            ret = "down";
            break;
          }
          case (Dir_left) {
            ret = "left";
            break;
          }
        }
        return ret;
      }
    )");
  });
    
  TEST(Loops, {
    ASSERT_COMPILES(R"(
      func breaks(param: sint) -> sint {
        var ret: sint;
        
        for (i := 0; i <= param; i++) {
          ret += i;
          if (i == 3) {
            ret *= 2;
            break;
          }
        }
        
        while (ret > 4) {
          if (ret % 3 == 0) break;
          ret /= 3;
        }
        
        return ret;
      }
      
      func continues(param: sint) -> sint {
        var ret = 1;
        
        for (i := param; i >= 0; i--) {
          if (i == 1) continue;
          ret *= i;
        }
        
        while (ret > 10) {
          ret -= 1;
          if (ret % 3 == 1) continue;
          ret -= 2;
        }
        
        return ret;
      }
    )");
  });
  
  TEST(Pass reference to reference, {
    ASSERT_COMPILES(R"(
      func swap_impl(a: ref sint, b: ref sint) {
        let temp = a;
        a = b;
        b = temp;
      }
      
      func swap(a: ref sint, b: ref sint) {
        swap_impl(a, b);
      }
      
      func identity(param: ref sint) {
        return param;
      }
      
      func test() {
        var two = 6;
        var six = 2;
        swap(two, six);
        var one = 1;
        let stillOne = identity(one);
      }
    )");
  })
  
  TEST(Closures, {
    ASSERT_COMPILES(R"(
      func fn(first: sint, second: real) -> bool {
        return make real first == second;
      }
      
      func test() {
        var a: func();
        var b: func() -> sint;
        var c: func(sint) -> sint;
        var d: func(sint, ref real) -> bool;
        let e = fn;
        
        if (e) {
          let tru = e(4, 4.0);
        }
      }
    )");
  });
  
  TEST(Lambda, {
    ASSERT_COMPILES(R"(
      func test() {
        let empty = func(){};
        let add = func(a: sint, b: sint) -> sint {
          return a + b;
        };
        let three = add(1, 2);
      }
    )");
  });
  
  TEST(Stateful lambda, {
    ASSERT_COMPILES(R"(
      func makeIDgen(first: sint) {
        return func() {
          let id = first;
          first++;
          return id;
        };
      }

      func test() {
        let gen = makeIDgen(4);
        let four = gen();
        let five = gen();
        let six = gen();
        let otherGen = gen;
        let seven = otherGen();
        let eight = gen();
        let nine = otherGen();
      }
    )");
  });
  
  TEST(Immediately invoked lambda, {
    ASSERT_COMPILES(R"(
      let nine = func() -> sint {
        return 4 + 5;
      }();
    )");
  });
  
  TEST(Nested lambda, {
    ASSERT_COMPILES(R"(
      // f_0
      func makeAdd(a: sint) {
        // lam_0: a - p_1
        // cap a has no index
        return func(b: sint) {
          // lam_1: a - c_0, b - p_1
          // cap a has index 0
          // cap b has no index
          return func(c: sint) {
            // c_0 + c_1 + p_1
            // a has index 0
            // b has index 1
            // c has no index
            return a + b + c;
          };
        };
      }
      
      func test() {
        let add_1 = makeAdd(1);
        let add_1_2 = add_1(2);
        let six = add_1_2(3);
      }
    )");
  });
  
  TEST(Nested nested lambda, {
    ASSERT_COMPILES(R"(
      // f_0
      func makeAdd(a: sint) {
        // lam_0: a - p_1
        // cap a has no index
        var other0 = 0;
        return func(b: uint) {
          // lam_1: a - c_0, b - p_1
          // cap a has index 0
          // cap b has no index
          other0++;
          var other1 = 1;
          var other2 = 2;
          a *= 2;
          return func(c: byte) {
            other0++;
            other1++;
            c = make byte (make sint c * 2);
            other2++;
            other1++;
            // lam_2: a - c_0, b - c_0, c - p_1
            // cap a has index 0
            // cap b has index 1
            // cap c has no index
            return func(d: char) {
              d *= 2c;
              other1++;
              b *= 2u;
              other0++;
              // c_0 + c_1 + c_2 + p_1
              // a has index 0
              // b has index 1
              // c has index 2
              // d has no index
              return make real a + make real b + make real c + make real d;
            };
          };
        };
      }
      
      func test() {
        let add_1 = makeAdd(1);
        let add_1_2 = add_1(2u);
        let add_1_2_3 = add_1_2(3b);
        let twenty = add_1_2_3(4c);
      }
    )");
  });
  */
});

#undef ASSERT_CPP_FAILS
#undef ASSERT_STELA_FAILS
#undef ASSERT_COMPILES
