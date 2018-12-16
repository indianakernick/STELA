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
#include <STELA/binding.hpp>
#include <STELA/code generation.hpp>
#include <STELA/syntax analysis.hpp>
#include <STELA/semantic analysis.hpp>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

using namespace stela;

namespace {

llvm::ExecutionEngine *generate(const stela::Symbols &syms, LogSink &log) {
  std::unique_ptr<llvm::Module> module = stela::generateIR(syms, log);
  llvm::Module *modulePtr = module.get();
  std::string str;
  llvm::raw_string_ostream strStream(str);
  strStream << *modulePtr;
  strStream.flush();
  std::cerr << "Generated IR\n" << str;
  str.clear();
  llvm::ExecutionEngine *engine = stela::generateCode(std::move(module), log);
  strStream << *modulePtr;
  strStream.flush();
  std::cerr << "Optimized IR\n" << str;
  return engine;
}

llvm::ExecutionEngine *generate(const std::string_view source, LogSink &log) {
  stela::AST ast = stela::createAST(source, log);
  stela::Symbols syms = stela::initModules(log);
  stela::compileModule(syms, ast, log);
  return generate(syms, log);
}

template <typename Fun>
auto getFunc(llvm::ExecutionEngine *engine, const std::string &name) {
  return stela::Function<Fun>{engine->getFunctionAddress(name)};
}

}

#define GET_FUNC(NAME, ...) getFunc<__VA_ARGS__>(engine, NAME)
#define ASSERT_SUCCEEDS(SOURCE) [[maybe_unused]] auto *engine = generate(SOURCE, log)
#define ASSERT_FAILS(SOURCE) ASSERT_THROWS(generate(SOURCE, log), stela::FatalError)

TEST_GROUP(Generation, {
  StreamSink stream;
  FilterSink log{stream, LogPri::verbose};
  
  TEST(Empty source, {
    ASSERT_SUCCEEDS("");
  });
  
  TEST(Function arguments, {
    ASSERT_SUCCEEDS(R"(
      extern func divide(a: real, b: real) -> real {
        return a / b;
      }
    )");
    auto func = GET_FUNC("divide", Real(Real, Real));
    ASSERT_EQ(func(10.0f, -2.0f), -5.0f);
    ASSERT_EQ(func(4.0f, 0.0f), std::numeric_limits<Real>::infinity());
  });
  
  TEST(Ref function arguments, {
    ASSERT_SUCCEEDS(R"(
      extern func swap(a: ref sint, b: ref sint) {
        let t = a;
        a = b;
        b = t;
      }
    )");
    auto func = GET_FUNC("swap", Void(Sint *, Sint *));
    Sint four = 7;
    Sint seven = 4;
    func(&four, &seven);
    ASSERT_EQ(four, 4);
    ASSERT_EQ(seven, 7);
  });
  
  TEST(For loop, {
    ASSERT_SUCCEEDS(R"(
      extern func multiply(a: uint, b: uint) -> uint {
        var product = 0u;
        for (; a != 0u; a = a - 1u) {
          product = product + b;
        }
        return product;
      }
    )");
    
    auto func = GET_FUNC("multiply", Uint(Uint, Uint));
    ASSERT_EQ(func(0, 0), 0);
    ASSERT_EQ(func(5, 0), 0);
    ASSERT_EQ(func(0, 5), 0);
    ASSERT_EQ(func(3, 4), 12);
    ASSERT_EQ(func(6, 6), 36);
  });
  
  TEST(Identity, {
    ASSERT_SUCCEEDS(R"(
      extern func identity(value: sint) -> sint {
        return value;
      }
    )");
    
    auto func = GET_FUNC("identity", Sint(Sint));
    ASSERT_EQ(func(0), 0);
    ASSERT_EQ(func(-11), -11);
    ASSERT_EQ(func(42), 42);
  });
  
  TEST(Switch with default, {
    ASSERT_SUCCEEDS(R"(
      extern func test(value: sint) -> real {
        switch (value) {
          case (0) {
            return 0.0;
          }
          case (1) {
            return 1.0;
          }
          case (2) {
            return 2.0;
          }
          default {
            return 3.0;
          }
        }
      }
    )");
    
    auto func = GET_FUNC("test", Real(Sint));
    ASSERT_EQ(func(0), 0.0f);
    ASSERT_EQ(func(1), 1.0f);
    ASSERT_EQ(func(2), 2.0f);
    ASSERT_EQ(func(3), 3.0f);
    ASSERT_EQ(func(789), 3.0f);
  });
  
  TEST(Switch without default, {
    ASSERT_SUCCEEDS(R"(
      extern func test(value: sint) -> real {
        let zero = 0;
        let one = 1;
        let two = 2;
        switch (value) {
          case (zero) return 0.0;
          case (one) return 1.0;
          case (two) return 2.0;
        }
        return 10.0;
      }
    )");
    
    auto func = GET_FUNC("test", Real(Sint));
    ASSERT_EQ(func(0), 0.0);
    ASSERT_EQ(func(1), 1.0);
    ASSERT_EQ(func(2), 2.0);
    ASSERT_EQ(func(3), 10.0);
    ASSERT_EQ(func(-7), 10.0);
  });
  
  TEST(Switch with fallthrough, {
    ASSERT_SUCCEEDS(R"(
      extern func test(value: sint) -> sint {
        var sum = 0;
        switch (value) {
          case (4) {
            sum = sum + 8;
            continue;
          }
          default {
            sum = sum - value;
            break;
          }
          case (16) {
            sum = sum + 2;
            continue;
          }
          case (8) {
            sum = sum + 4;
          }
        }
        return sum;
      }
    )");
    
    auto func = GET_FUNC("test", Sint(Sint));
    ASSERT_EQ(func(4), 4);
    ASSERT_EQ(func(8), 4);
    ASSERT_EQ(func(16), 6);
    ASSERT_EQ(func(32), -32);
    ASSERT_EQ(func(0), 0);
  });
  
  TEST(If-else, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) -> real {
        if (val < -8) {
          return -1.0;
        } else if (val > 8) {
          return 1.0;
        } else {
          return 0.0;
        }
      }
    )");
    
    auto func = GET_FUNC("test", Real(Sint));
    ASSERT_EQ(func(-50), -1.0f);
    ASSERT_EQ(func(3), 0.0f);
    ASSERT_EQ(func(22), 1.0f);
    ASSERT_EQ(func(-8), 0.0f);
    ASSERT_EQ(func(8), 0.0f);
  });
  
  TEST(If-else no return, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) -> real {
        var ret: real;
        if (val < -8) {
          ret = -1.0;
        } else if (val > 8) {
          ret = 1.0;
        } else {
          ret = 0.0;
        }
        return ret;
      }
    )");
    
    auto func = GET_FUNC("test", Real(Sint));
    ASSERT_EQ(func(-50), -1.0f);
    ASSERT_EQ(func(3), 0.0f);
    ASSERT_EQ(func(22), 1.0f);
    ASSERT_EQ(func(-8), 0.0f);
    ASSERT_EQ(func(8), 0.0f);
  });
  
  TEST(If, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) -> real {
        if (val < 0) {
          return -1.0;
        }
        return 1.0;
      }
    )");
    
    auto func = GET_FUNC("test", Real(Sint));
    ASSERT_EQ(func(2), 1.0);
    ASSERT_EQ(func(0), 1.0);
    ASSERT_EQ(func(-1), -1.0);
    ASSERT_EQ(func(-5), -1.0);
  });
  
  TEST(If no return, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) -> real {
        var ret = 1.0;
        if (val < 0) {
          ret = -1.0;
        }
        return ret;
      }
    )");
    
    auto func = GET_FUNC("test", Real(Sint));
    ASSERT_EQ(func(2), 1.0);
    ASSERT_EQ(func(0), 1.0);
    ASSERT_EQ(func(-1), -1.0);
    ASSERT_EQ(func(-5), -1.0);
  });
  
  TEST(Ternary conditional, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) -> sint {
        return val < 0 ? -val : val;
      }
    )");
    
    auto func = GET_FUNC("test", Sint(Sint));
    ASSERT_EQ(func(2), 2);
    ASSERT_EQ(func(1), 1);
    ASSERT_EQ(func(0), 0);
    ASSERT_EQ(func(-1), 1);
    ASSERT_EQ(func(-2), 2);
  });
  
  TEST(Ternary conditional lvalue, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) -> sint {
        var a = 2;
        var b = 4;
        (val < 0 ? a : b) = val;
        return a + b;
      }
    )");
    
    auto func = GET_FUNC("test", Sint(Sint));
    ASSERT_EQ(func(3), 5);
    ASSERT_EQ(func(0), 2);
    ASSERT_EQ(func(-1), 3);
    ASSERT_EQ(func(-8), -4);
  });
  
  TEST(Function call, {
    ASSERT_SUCCEEDS(R"(
      func helper(a: sint) {
        return a * 2;
      }
      func helper(a: sint, b: sint) {
        return helper(a) + b * 4;
      }
      func swap(a: ref sint, b: ref sint) {
        let t = a;
        a = b;
        b = t;
      }
      
      extern func test(val: sint) {
        var a = 4;
        swap(a, val);
        return helper(a, val);
      }
    )");
    
    auto func = GET_FUNC("test", Sint(Sint));
    ASSERT_EQ(func(-8), 0);
    ASSERT_EQ(func(0), 16);
    ASSERT_EQ(func(8), 32);
  });
  
  TEST(Void function call, {
    ASSERT_SUCCEEDS(R"(
      func increase(val: ref real) {
        val = val * 2.0;
      }
      func increaseMore(val: ref real) {
        increase(val);
        return increase(val);
      }
      
      extern func test(val: real) {
        val = val + 4.0;
        increaseMore(val);
        return val;
      }
    )");
    
    auto func = GET_FUNC("test", Real(Real));
    ASSERT_EQ(func(-4.0f), 0.0f);
    ASSERT_EQ(func(0.0f), 16.0f);
    ASSERT_EQ(func(4.0f), 32.0f);
  });
  
  TEST(Member function call, {
    ASSERT_SUCCEEDS(R"(
      func (a: sint) helper() {
        return a * 2;
      }
      func (a: sint) helper(b: sint) {
        return a.helper() + b * 4;
      }
      func (a: ref sint) swap(b: ref sint) {
        let t = a;
        a = b;
        b = t;
      }
    
      extern func test(val: sint) {
        var a = 4;
        a.swap(val);
        return a.helper(val);
      }
    )");
    
    auto func = GET_FUNC("test", Sint(Sint));
    ASSERT_EQ(func(-8), 0);
    ASSERT_EQ(func(0), 16);
    ASSERT_EQ(func(8), 32);
  });
  
  TEST(Void member function call, {
    ASSERT_SUCCEEDS(R"(
      func (val: ref real) increase() {
        val = val * 2.0;
      }
      func (val: ref real) increaseMore() {
        val.increase();
        return val.increase();
      }
      
      extern func test(val: real) {
        val = val + 4.0;
        val.increaseMore();
        return val;
      }
    )");
    
    auto func = GET_FUNC("test", Real(Real));
    ASSERT_EQ(func(-4.0f), 0.0f);
    ASSERT_EQ(func(0.0f), 16.0f);
    ASSERT_EQ(func(4.0f), 32.0f);
  });
  
  TEST(Nested for loop, {
    ASSERT_SUCCEEDS(R"(
      extern func test(a: uint, b: uint) -> uint {
        var product = 0u;
        for (i := 0u; i != a; i = i + 1u) {
          for (j := 0u; j != b; j = j + 1u) {
            product = product + 1u;
          }
        }
        return product;
      }
    )");
    
    auto func = GET_FUNC("test", Uint(Uint, Uint));
    ASSERT_EQ(func(0, 0), 0);
    ASSERT_EQ(func(0, 4), 0);
    ASSERT_EQ(func(4, 0), 0);
    ASSERT_EQ(func(1, 1), 1);
    ASSERT_EQ(func(3, 4), 12);
    ASSERT_EQ(func(6, 6), 36);
    ASSERT_EQ(func(9, 5), 45);
  });
  
  TEST(Nested nested for loop, {
    ASSERT_SUCCEEDS(R"(
      extern func test(a: uint, b: uint, c: uint) -> uint {
        var product = 0u;
        for (i := 0u; i != a; i = i + 1u) {
          for (j := 0u; j != b; j = j + 1u) {
            for (k := 0u; k != c; k = k + 1u) {
              product = product + 1u;
            }
          }
        }
        return product;
      }
    )");
    
    auto func = GET_FUNC("test", Uint(Uint, Uint, Uint));
    ASSERT_EQ(func(0, 0, 0), 0);
    ASSERT_EQ(func(0, 0, 4), 0);
    ASSERT_EQ(func(0, 5, 0), 0);
    ASSERT_EQ(func(6, 0, 0), 0);
    ASSERT_EQ(func(2, 2, 2), 8);
    ASSERT_EQ(func(2, 4, 8), 64);
    ASSERT_EQ(func(1, 1, 1), 1);
  });
  
  TEST(Sequential for loop, {
    ASSERT_SUCCEEDS(R"(
      extern func test(a: uint, b: uint) -> uint {
        var sum = 0u;
        for (i := 0u; i != a; i = i + 1u) {
          sum = sum + 1u;
        }
        for (i := 0u; i != b; i = i + 1u) {
          sum = sum + 1u;
        }
        return sum;
      }
    )");
    
    auto func = GET_FUNC("test", Uint(Uint, Uint));
    ASSERT_EQ(func(0, 0), 0);
    ASSERT_EQ(func(0, 4), 4);
    ASSERT_EQ(func(5, 0), 5);
    ASSERT_EQ(func(9, 10), 19);
    ASSERT_EQ(func(12000, 321), 12321);
  });
  
  TEST(Returning while loop, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) {
        while (val > 8) {
          if (val % 2 == 1) {
            val = val - 1;
          }
          return val;
        }
        return val * 2;
      }
    )");
    
    auto func = GET_FUNC("test", Sint(Sint));
    ASSERT_EQ(func(8), 16);
    ASSERT_EQ(func(16), 16);
    ASSERT_EQ(func(17), 16);
    ASSERT_EQ(func(4), 8);
    ASSERT_EQ(func(9), 8);
  });
  
  TEST(Bool across boundary, {
    ASSERT_SUCCEEDS(R"(
      extern func set(val: bool) {
        if (val == false) {
          return 0;
        } else if (val == true) {
          return 1;
        } else {
          return 2;
        }
      }
      
      extern func get(val: sint) {
        if (val == 0) {
          return false;
        } else {
          return true;
        }
      }
      
      extern func not(val: bool) {
        return !val;
      }
    )");
    
    auto set = GET_FUNC("set", Sint(Bool));
    ASSERT_EQ(set(false), 0);
    ASSERT_EQ(set(true), 1);
    
    auto get = GET_FUNC("get", Bool(Sint));
    ASSERT_EQ(get(0), false);
    ASSERT_EQ(get(1), true);
    
    auto bool_not = GET_FUNC("not", Bool(Bool));
    ASSERT_EQ(bool_not(false), true);
    ASSERT_EQ(bool_not(true), false);
  });
  
  TEST(Compound assignments, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) {
        val *= 2;
        val += 2;
        val /= 2;
        return val;
      }
    )");
    
    auto func = GET_FUNC("test", Sint(Sint));
    ASSERT_EQ(func(0), 1);
    ASSERT_EQ(func(1), 2);
    ASSERT_EQ(func(2), 3);
    ASSERT_EQ(func(41), 42);
    ASSERT_EQ(func(-13), -12);
  });
  
  TEST(Incr decr assignments, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: sint) {
        val++;
        val++;
        val++;
        val--;
        val--;
        val++;
        return val;
      }
    )");
    
    auto func = GET_FUNC("test", Sint(Sint));
    ASSERT_EQ(func(0), 2);
    ASSERT_EQ(func(-2), 0);
    ASSERT_EQ(func(55), 57);
    ASSERT_EQ(func(40), 42);
  });
  
  TEST(Incr decr assignments floats, {
    ASSERT_SUCCEEDS(R"(
      extern func test(val: real) {
        val++;
        val++;
        val++;
        val--;
        val--;
        val++;
        return val;
      }
    )");
    
    auto func = GET_FUNC("test", Real(Real));
    ASSERT_EQ(func(0), 2);
    ASSERT_EQ(func(-2), 0);
    ASSERT_EQ(func(55), 57);
    ASSERT_EQ(func(40), 42);
  });
  
  TEST(Structs across boundary, {
    ASSERT_SUCCEEDS(R"(
      type Structure struct {
        a: byte;
        b: byte;
        c: byte;
        d: byte;
        e: real;
        f: char;
        g: sint;
      };
      
      extern func set(s: ref Structure, v: sint) {
        s.g = v;
      }
      extern func get(s: ref Structure) {
        return s.g;
      }
      extern func get_val(s: Structure) {
        return s.g;
      }
      func identity_impl(s: Structure) {
        let temp = s;
        return temp;
      }
      extern func identity(s: Structure) {
        return identity_impl(s);
      }
    )");
    
    struct Structure {
      Byte a;
      Byte b;
      Byte c;
      Byte d;
      Real e;
      Char f;
      Sint g;
    } s;
    
    auto set = GET_FUNC("set", Void(Structure *, Sint));
    
    s.g = 0;
    set(&s, 4);
    ASSERT_EQ(s.g, 4);
    set(&s, -100000);
    ASSERT_EQ(s.g, -100000);
    
    auto get = GET_FUNC("get", Sint(Structure *));
    
    s.g = 4;
    ASSERT_EQ(get(&s), 4);
    s.g = -100000;
    ASSERT_EQ(get(&s), -100000);
    
    auto get_val = GET_FUNC("get_val", Sint(Structure *));
    
    s.g = 4;
    ASSERT_EQ(get_val(&s), 4);
    s.g = -100000;
    ASSERT_EQ(get_val(&s), -100000);
    
    auto identity = GET_FUNC("identity", Structure(Structure));
    
    s.g = 4;
    ASSERT_EQ(identity(s).g, s.g);
    s.g = -100000;
    ASSERT_EQ(identity(s).g, s.g);
  });
  
  TEST(Init struct, {
    ASSERT_SUCCEEDS(R"(
      type Structure struct {
        a: byte;
        b: byte;
        c: byte;
        d: byte;
        e: real;
        f: char;
        g: sint;
      };
      
      extern func zero() {
        var a: Structure;
        let b: Structure = {};
        let c = make Structure {};
        return a.e + b.e + c.e;
      }
      
      extern func init() {
        return make Structure {
          1b,
          make byte 2,
          make byte 3b,
          4b,
          make real 5,
          6c,
          make sint 7u
        };
      }
    )");
    
    auto zero = GET_FUNC("zero", Real());
    
    ASSERT_EQ(zero(), 0.0f);
    
    struct Structure {
      Byte a;
      Byte b;
      Byte c;
      Byte d;
      Real e;
      Char f;
      Sint g;
    };
    
    auto init = GET_FUNC("init", Structure());
    
    Structure s = init();
    ASSERT_EQ(s.a, 1);
    ASSERT_EQ(s.b, 2);
    ASSERT_EQ(s.c, 3);
    ASSERT_EQ(s.d, 4);
    ASSERT_EQ(s.e, 5);
    ASSERT_EQ(s.f, 6);
    ASSERT_EQ(s.g, 7);
  });
  
  TEST(Casting, {
    ASSERT_SUCCEEDS(R"(
      extern func toUint(val: sint) {
        return make uint val;
      }
    
      extern func toSint(val: real) {
        return make sint val;
      }
      
      extern func toReal(val: sint) {
        return make real val;
      }
    )");
    
    auto toUint = GET_FUNC("toUint", Uint(Sint));
    
    ASSERT_EQ(toUint(4), 4);
    ASSERT_EQ(toUint(100000), 100000);
    ASSERT_EQ(toUint(-9), Uint(-9));
    
    auto toSint = GET_FUNC("toSint", Sint(Real));
    
    ASSERT_EQ(toSint(4.0f), 4);
    ASSERT_EQ(toSint(100.7f), 100);
    ASSERT_EQ(toSint(-9.4f), -9);
    
    auto toReal = GET_FUNC("toReal", Real(Sint));
    
    ASSERT_EQ(toReal(4), 4.0f);
    ASSERT_EQ(toReal(100000), 100000.0f);
    ASSERT_EQ(toReal(-9), -9.0f);
  });
  
  TEST(Recursive function, {
    ASSERT_SUCCEEDS(R"(
      extern func fac(n: uint) -> uint {
        return n == 0u ? 1u : n * fac(n - 1u);
      }
    )");
    
    auto fac = GET_FUNC("fac", Uint(Uint));
    
    ASSERT_EQ(fac(0), 1);
    ASSERT_EQ(fac(1), 1);
    ASSERT_EQ(fac(2), 2);
    ASSERT_EQ(fac(3), 6);
    ASSERT_EQ(fac(4), 24);
    ASSERT_EQ(fac(5), 120);
    ASSERT_EQ(fac(6), 720);
    ASSERT_EQ(fac(7), 5'040);
    ASSERT_EQ(fac(8), 40'320);
    ASSERT_EQ(fac(9), 362'880);
    ASSERT_EQ(fac(10), 3'628'800);
    ASSERT_EQ(fac(11), 39'916'800);
    ASSERT_EQ(fac(12), 479'001'600);
    // fac(13) overflows uint32_t
  });
  
  /*
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

#undef ASSERT_FAILS
#undef ASSERT_SUCCEEDS
#undef GET_FUNC
