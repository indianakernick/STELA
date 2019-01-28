//
//  generation.cpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include <fstream>
#include <iostream>
#include <gtest/gtest.h>
#include <STELA/llvm.hpp>
#include <llvm/IR/Module.h>
#include <STELA/binding.hpp>
#include <STELA/reflection.hpp>
#include <STELA/code generation.hpp>
#include <STELA/syntax analysis.hpp>
#include <STELA/native functions.hpp>
#include <STELA/semantic analysis.hpp>
#include <STELA/c standard library.hpp>

using namespace stela;

namespace {

class LLVMEnvionment : public ::testing::Environment {
public:
  void SetUp() override {
    stela::initLLVM();
  }
  void TearDown() override {
    stela::quitLLVM();
  }
};

::testing::Environment *env = ::testing::AddGlobalTestEnvironment(new LLVMEnvionment);

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

LogSink &log() {
  static StreamSink stream;
  static FilterSink filter{stream, LogPri::info};
  return filter;
}

#define GET_FUNC(NAME, ...) getFunc<__VA_ARGS__>(engine, NAME)
#define GET_MEM_FUNC(NAME, ...) getFunc<__VA_ARGS__, true>(engine, NAME)
#define EXPECT_SUCCEEDS(SOURCE) [[maybe_unused]] auto *engine = generate(SOURCE, log())
#define EXPECT_FAILS(SOURCE) EXPECT_THROWS(generate(SOURCE, log()), stela::FatalError)

TEST(Basic, Empty_source) {
  EXPECT_SUCCEEDS("");
}

TEST(Func, Arguments) {
  EXPECT_SUCCEEDS(R"(
    extern func divide(a: real, b: real) -> real {
      return a / b;
    }
  )");
  auto func = GET_FUNC("divide", Real(Real, Real));
  EXPECT_EQ(func(10.0f, -2.0f), -5.0f);
  EXPECT_EQ(func(4.0f, 0.0f), std::numeric_limits<Real>::infinity());
}

TEST(Func, Ref_arguments) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(four, 4);
  EXPECT_EQ(seven, 7);
}

TEST(Loops, For_loop) {
  EXPECT_SUCCEEDS(R"(
    extern func multiply(a: uint, b: uint) -> uint {
      var product = 0u;
      for (; a != 0u; a = a - 1u) {
        product = product + b;
      }
      return product;
    }
  )");
  
  auto func = GET_FUNC("multiply", Uint(Uint, Uint));
  EXPECT_EQ(func(0u, 0u), 0u);
  EXPECT_EQ(func(5u, 0u), 0u);
  EXPECT_EQ(func(0u, 5u), 0u);
  EXPECT_EQ(func(3u, 4u), 12u);
  EXPECT_EQ(func(6u, 6u), 36u);
}

TEST(Expr, Identity) {
  EXPECT_SUCCEEDS(R"(
    extern func identity(value: sint) -> sint {
      return value;
    }
  )");
  
  auto func = GET_FUNC("identity", Sint(Sint));
  EXPECT_EQ(func(0), 0);
  EXPECT_EQ(func(-11), -11);
  EXPECT_EQ(func(42), 42);
}

TEST(Switch, With_default) {
  EXPECT_SUCCEEDS(R"(
    extern func test(value: sint) -> real {
      switch value {
        case 0 {
          return 0.0;
        }
        case 1 {
          return 1.0;
        }
        case 2 {
          return 2.0;
        }
        default {
          return 3.0;
        }
      }
    }
  )");
  
  auto func = GET_FUNC("test", Real(Sint));
  EXPECT_EQ(func(0), 0.0f);
  EXPECT_EQ(func(1), 1.0f);
  EXPECT_EQ(func(2), 2.0f);
  EXPECT_EQ(func(3), 3.0f);
  EXPECT_EQ(func(789), 3.0f);
}

TEST(Switch, Without_default) {
  EXPECT_SUCCEEDS(R"(
    extern func test(value: sint) -> real {
      let zero = 0;
      let one = 1;
      let two = 2;
      switch value {
        case zero return 0.0;
        case one return 1.0;
        case two return 2.0;
      }
      return 10.0;
    }
  )");
  
  auto func = GET_FUNC("test", Real(Sint));
  EXPECT_EQ(func(0), 0.0);
  EXPECT_EQ(func(1), 1.0);
  EXPECT_EQ(func(2), 2.0);
  EXPECT_EQ(func(3), 10.0);
  EXPECT_EQ(func(-7), 10.0);
}

TEST(Switch, With_fallthrough) {
  EXPECT_SUCCEEDS(R"(
    extern func test(value: sint) -> sint {
      var sum = 0;
      switch value {
        case 4 {
          sum = sum + 8;
          continue;
        }
        default {
          sum = sum - value;
          break;
        }
        case 16 {
          sum = sum + 2;
          continue;
        }
        case 8 {
          sum = sum + 4;
        }
      }
      return sum;
    }
  )");
  
  auto func = GET_FUNC("test", Sint(Sint));
  EXPECT_EQ(func(4), 4);
  EXPECT_EQ(func(8), 4);
  EXPECT_EQ(func(16), 6);
  EXPECT_EQ(func(32), -32);
  EXPECT_EQ(func(0), 0);
}

TEST(If, Else) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(-50), -1.0f);
  EXPECT_EQ(func(3), 0.0f);
  EXPECT_EQ(func(22), 1.0f);
  EXPECT_EQ(func(-8), 0.0f);
  EXPECT_EQ(func(8), 0.0f);
}

TEST(If, Else_no_return) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(-50), -1.0f);
  EXPECT_EQ(func(3), 0.0f);
  EXPECT_EQ(func(22), 1.0f);
  EXPECT_EQ(func(-8), 0.0f);
  EXPECT_EQ(func(8), 0.0f);
}

TEST(If, Return) {
  EXPECT_SUCCEEDS(R"(
    extern func test(val: sint) -> real {
      if (val < 0) {
        return -1.0;
      }
      return 1.0;
    }
  )");
  
  auto func = GET_FUNC("test", Real(Sint));
  EXPECT_EQ(func(2), 1.0);
  EXPECT_EQ(func(0), 1.0);
  EXPECT_EQ(func(-1), -1.0);
  EXPECT_EQ(func(-5), -1.0);
}

TEST(If, No_return) {
  EXPECT_SUCCEEDS(R"(
    extern func test(val: sint) -> real {
      var ret = 1.0;
      if (val < 0) {
        ret = -1.0;
      }
      return ret;
    }
  )");
  
  auto func = GET_FUNC("test", Real(Sint));
  EXPECT_EQ(func(2), 1.0);
  EXPECT_EQ(func(0), 1.0);
  EXPECT_EQ(func(-1), -1.0);
  EXPECT_EQ(func(-5), -1.0);
}

TEST(Ternary, Regular) {
  EXPECT_SUCCEEDS(R"(
    extern func test(val: sint) -> sint {
      return val < 0 ? -val : val;
    }
  )");
  
  auto func = GET_FUNC("test", Sint(Sint));
  EXPECT_EQ(func(2), 2);
  EXPECT_EQ(func(1), 1);
  EXPECT_EQ(func(0), 0);
  EXPECT_EQ(func(-1), 1);
  EXPECT_EQ(func(-2), 2);
}

TEST(Ternary, Lvalue) {
  EXPECT_SUCCEEDS(R"(
    extern func test(val: sint) -> sint {
      var a = 2;
      var b = 4;
      (val < 0 ? a : b) = val;
      return a + b;
    }
  )");
  
  auto func = GET_FUNC("test", Sint(Sint));
  EXPECT_EQ(func(3), 5);
  EXPECT_EQ(func(0), 2);
  EXPECT_EQ(func(-1), 3);
  EXPECT_EQ(func(-8), -4);
}

TEST(Ternary, Nontrivial) {
  EXPECT_SUCCEEDS(R"(
    extern func select(identity: bool, array: [real]) {
      return identity ? array : make [real] {};
    }
    
    extern func selectTemp(identity: bool, array: [real]) {
      var temp = make [real] {};
      temp = identity ? array : make [real] {};
      return temp;
    }
  )");
  
  auto select = GET_FUNC("select", Array<Real>(Bool, Array<Real>));
  {
    Array<Real> array = makeEmptyArray<Real>();
    Array<Real> sameArray = select(true, array);
    ASSERT_TRUE(array);
    ASSERT_TRUE(sameArray);
    EXPECT_EQ(array, sameArray);
    EXPECT_EQ(array.use_count(), 2);
    
    Array<Real> array2 = makeEmptyArray<Real>();
    Array<Real> diffArray = select(false, array2);
    ASSERT_TRUE(array2);
    ASSERT_TRUE(diffArray);
    EXPECT_NE(array2, diffArray);
    EXPECT_EQ(array2.use_count(), 1);
    EXPECT_EQ(diffArray.use_count(), 1);
  }
  
  auto selectTemp = GET_FUNC("selectTemp", Array<Real>(Bool, Array<Real>));
  {
    Array<Real> array = makeEmptyArray<Real>();
    Array<Real> sameArray = selectTemp(true, array);
    ASSERT_TRUE(array);
    ASSERT_TRUE(sameArray);
    EXPECT_EQ(array, sameArray);
    EXPECT_EQ(array.use_count(), 2);
    
    Array<Real> array2 = makeEmptyArray<Real>();
    Array<Real> diffArray = selectTemp(false, array2);
    ASSERT_TRUE(array2);
    ASSERT_TRUE(diffArray);
    EXPECT_NE(array2, diffArray);
    EXPECT_EQ(array2.use_count(), 1);
    EXPECT_EQ(diffArray.use_count(), 1);
  }
}

TEST(Func, Call) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(-8), 0);
  EXPECT_EQ(func(0), 16);
  EXPECT_EQ(func(8), 32);
}

TEST(Func, Void_call) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(-4.0f), 0.0f);
  EXPECT_EQ(func(0.0f), 16.0f);
  EXPECT_EQ(func(4.0f), 32.0f);
}

TEST(Func, Member_function_call) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(-8), 0);
  EXPECT_EQ(func(0), 16);
  EXPECT_EQ(func(8), 32);
}

TEST(Func, Void_method_call) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(-4.0f), 0.0f);
  EXPECT_EQ(func(0.0f), 16.0f);
  EXPECT_EQ(func(4.0f), 32.0f);
}

TEST(Loops, Nested_for_loop) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(0u, 0u), 0u);
  EXPECT_EQ(func(0u, 4u), 0u);
  EXPECT_EQ(func(4u, 0u), 0u);
  EXPECT_EQ(func(1u, 1u), 1u);
  EXPECT_EQ(func(3u, 4u), 12u);
  EXPECT_EQ(func(6u, 6u), 36u);
  EXPECT_EQ(func(9u, 5u), 45u);
}

TEST(Loops, Nested_nested_for_loop) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(0u, 0u, 0u), 0u);
  EXPECT_EQ(func(0u, 0u, 4u), 0u);
  EXPECT_EQ(func(0u, 5u, 0u), 0u);
  EXPECT_EQ(func(6u, 0u, 0u), 0u);
  EXPECT_EQ(func(2u, 2u, 2u), 8u);
  EXPECT_EQ(func(2u, 4u, 8u), 64u);
  EXPECT_EQ(func(1u, 1u, 1u), 1u);
}

TEST(Loops, Sequential_for_loop) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(0u, 0u), 0u);
  EXPECT_EQ(func(0u, 4u), 4u);
  EXPECT_EQ(func(5u, 0u), 5u);
  EXPECT_EQ(func(9u, 10u), 19u);
  EXPECT_EQ(func(12000u, 321u), 12321u);
}

TEST(Loops, Returning_while_loop) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(8), 16);
  EXPECT_EQ(func(16), 16);
  EXPECT_EQ(func(17), 16);
  EXPECT_EQ(func(4), 8);
  EXPECT_EQ(func(9), 8);
}

TEST(Binding, Bool) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(set(false), 0);
  EXPECT_EQ(set(true), 1);
  
  auto get = GET_FUNC("get", Bool(Sint));
  EXPECT_EQ(get(0), false);
  EXPECT_EQ(get(1), true);
  
  auto bool_not = GET_FUNC("not", Bool(Bool));
  EXPECT_EQ(bool_not(false), true);
  EXPECT_EQ(bool_not(true), false);
}

TEST(Assign, Compound) {
  EXPECT_SUCCEEDS(R"(
    extern func test(val: sint) {
      val *= 2;
      val += 2;
      val /= 2;
      return val;
    }
  )");
  
  auto func = GET_FUNC("test", Sint(Sint));
  EXPECT_EQ(func(0), 1);
  EXPECT_EQ(func(1), 2);
  EXPECT_EQ(func(2), 3);
  EXPECT_EQ(func(41), 42);
  EXPECT_EQ(func(-13), -12);
}

TEST(Assign, Incr_decr) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(0), 2);
  EXPECT_EQ(func(-2), 0);
  EXPECT_EQ(func(55), 57);
  EXPECT_EQ(func(40), 42);
}

TEST(Assign, Incr_decr_floats) {
  EXPECT_SUCCEEDS(R"(
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
  EXPECT_EQ(func(0), 2);
  EXPECT_EQ(func(-2), 0);
  EXPECT_EQ(func(55), 57);
  EXPECT_EQ(func(40), 42);
}

TEST(Binding, Structs) {
  EXPECT_SUCCEEDS(R"(
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
    func (s: Structure) identity_impl() {
      let temp = s;
      return temp;
    }
    extern func identity(s: Structure) {
      return s.identity_impl();
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
  EXPECT_EQ(s.g, 4);
  set(&s, -100000);
  EXPECT_EQ(s.g, -100000);
  
  auto get = GET_FUNC("get", Sint(Structure *));
  
  s.g = 4;
  EXPECT_EQ(get(&s), 4);
  s.g = -100000;
  EXPECT_EQ(get(&s), -100000);
  
  auto get_val = GET_FUNC("get_val", Sint(Structure *));
  
  s.g = 4;
  EXPECT_EQ(get_val(&s), 4);
  s.g = -100000;
  EXPECT_EQ(get_val(&s), -100000);
  
  auto identity = GET_FUNC("identity", Structure(Structure));
  
  s.g = 4;
  EXPECT_EQ(identity(s).g, s.g);
  s.g = -100000;
  EXPECT_EQ(identity(s).g, s.g);
}

TEST(Expr, Init_struct) {
  EXPECT_SUCCEEDS(R"(
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
      return a.e + b.e + c.e + (make Structure {}).e;
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
  
  EXPECT_EQ(zero(), 0.0f);
  
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
  EXPECT_EQ(s.a, 1);
  EXPECT_EQ(s.b, 2);
  EXPECT_EQ(s.c, 3);
  EXPECT_EQ(s.d, 4);
  EXPECT_EQ(s.e, 5);
  EXPECT_EQ(s.f, 6);
  EXPECT_EQ(s.g, 7);
}

TEST(Expr, Casting) {
  EXPECT_SUCCEEDS(R"(
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
  
  EXPECT_EQ(toUint(4), 4);
  EXPECT_EQ(toUint(100000), 100000);
  EXPECT_EQ(toUint(-9), Uint(-9));
  
  auto toSint = GET_FUNC("toSint", Sint(Real));
  
  EXPECT_EQ(toSint(4.0f), 4);
  EXPECT_EQ(toSint(100.7f), 100);
  EXPECT_EQ(toSint(-9.4f), -9);
  
  auto toReal = GET_FUNC("toReal", Real(Sint));
  
  EXPECT_EQ(toReal(4), 4.0f);
  EXPECT_EQ(toReal(100000), 100000.0f);
  EXPECT_EQ(toReal(-9), -9.0f);
}

TEST(Func, Recursive) {
  EXPECT_SUCCEEDS(R"(
    extern func fac(n: uint) -> uint {
      return n == 0u ? 1u : n * fac(n - 1u);
    }
  )");
  
  auto fac = GET_FUNC("fac", Uint(Uint));
  
  EXPECT_EQ(fac(0u), 1u);
  EXPECT_EQ(fac(1u), 1u);
  EXPECT_EQ(fac(2u), 2u);
  EXPECT_EQ(fac(3u), 6u);
  EXPECT_EQ(fac(4u), 24u);
  EXPECT_EQ(fac(5u), 120u);
  EXPECT_EQ(fac(6u), 720u);
  EXPECT_EQ(fac(7u), 5'040u);
  EXPECT_EQ(fac(8u), 40'320u);
  EXPECT_EQ(fac(9u), 362'880u);
  EXPECT_EQ(fac(10u), 3'628'800u);
  EXPECT_EQ(fac(11u), 39'916'800u);
  EXPECT_EQ(fac(12u), 479'001'600u);
  // fac(13) overflows uint32_t
}

TEST(Lifetime, Global_variables) {
  EXPECT_SUCCEEDS(R"(
    let five = 5;
    
    extern func getFive() {
      return five;
    }
    
    type Dir sint;
    let Dir_up = make Dir 0;
    let Dir_right = make Dir 1;
    let Dir_down = make Dir 2;
    let Dir_left = make Dir 3;
    
    extern func classify(dir: Dir) {
      switch dir {
        case Dir_up return 11;
        case Dir_right return 22;
        case Dir_down return 33;
        case Dir_left return 44;
        default return 0;
      }
    }
  )");
  
  auto getFive = GET_FUNC("getFive", Sint());
  
  EXPECT_EQ(getFive(), 5);
  
  auto classify = GET_FUNC("classify", Sint(Sint));
  
  EXPECT_EQ(classify(0), 11);
  EXPECT_EQ(classify(1), 22);
  EXPECT_EQ(classify(2), 33);
  EXPECT_EQ(classify(3), 44);
  EXPECT_EQ(classify(4), 0);
  EXPECT_EQ(classify(-100), 0);
}

TEST(Binding, Arrays) {
  EXPECT_SUCCEEDS(R"(
    extern func get() {
      var a: [real];
      var b = make [real] {};
      a = a;
      a = b;
      return a;
    }
    
    extern func identity(a: [real]) {
      return a;
    }
    
    extern func setFirst(a: [real], val: real) {
      a[0] = val;
    }
    
    extern func getFirst(a: [real]) {
      return a[0u];
    }
  )");
  
  auto get = GET_FUNC("get", Array<Real>());
  
  Array<Real> array = get();
  ASSERT_TRUE(array);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->cap, 0);
  EXPECT_EQ(array->len, 0);
  EXPECT_EQ(array->dat, nullptr);
  
  auto identity = GET_FUNC("identity", Array<Real>(Array<Real>));
  
  Array<Real> orig = makeEmptyArray<Real>();
  Array<Real> copy = identity(orig);
  ASSERT_TRUE(orig);
  ASSERT_TRUE(copy);
  EXPECT_EQ(orig.get(), copy.get());
  EXPECT_EQ(orig.use_count(), 2);
  
  Array<Real> same = identity(makeEmptyArray<Real>());
  ASSERT_TRUE(same);
  EXPECT_EQ(same.use_count(), 1);

  auto setFirst = GET_FUNC("setFirst", Void(Array<Real>, Real));

  Array<Real> array1 = makeArray<Real>(1);
  EXPECT_EQ(array1.use_count(), 1);

  setFirst(array1, 11.5f);
  EXPECT_EQ(array1.use_count(), 1);
  EXPECT_EQ(array1->cap, 1);
  EXPECT_EQ(array1->len, 1);
  ASSERT_TRUE(array1->dat);
  EXPECT_EQ(array1->dat[0], 11.5f);
  
  auto getFirst = GET_FUNC("getFirst", Real(Array<Real>));
  
  const Real first = getFirst(array1);
  EXPECT_EQ(array1.use_count(), 1);
  EXPECT_EQ(array1->cap, 1);
  EXPECT_EQ(array1->len, 1);
  ASSERT_TRUE(array1->dat);
  EXPECT_EQ(array1->dat[0], 11.5f);
  EXPECT_EQ(first, 11.5f);
}

TEST(Assign, Structs) {
  EXPECT_SUCCEEDS(R"(
    type S struct {
      m: [real];
    };
    
    extern func get() {
      var s = make S {};
      s = make S {};
      return s.m;
    }
  )");
  
  auto get = GET_FUNC("get", Array<Real>());
  
  Array<Real> array = get();
  ASSERT_TRUE(array);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->cap, 0);
  EXPECT_EQ(array->len, 0);
  EXPECT_EQ(array->dat, nullptr);
}

TEST(Lifetime, Destructors_in_switch) {
  EXPECT_SUCCEEDS(R"(
    extern func get_1_ref(val: sint) {
      var array: [real];
      switch val {
        case 0 {
          let retain = array;
          {
            continue;
          }
        }
        case 1 {
          break;
        }
        case 2 {
          return array;
        }
        default {
          let retain0 = array;
          {
            let retain1 = array;
            return array;
          }
        }
      }
      return array;
    }
  )");
  
  auto get = GET_FUNC("get_1_ref", Array<Real>(Sint));
  
  Array<Real> a = get(0);
  ASSERT_TRUE(a);
  EXPECT_EQ(a.use_count(), 1);
  
  Array<Real> b = get(1);
  ASSERT_TRUE(b);
  EXPECT_EQ(b.use_count(), 1);
  
  Array<Real> c = get(2);
  ASSERT_TRUE(c);
  EXPECT_EQ(c.use_count(), 1);
  
  Array<Real> d = get(3);
  ASSERT_TRUE(d);
  EXPECT_EQ(d.use_count(), 1);
}

TEST(Switch, Strings) {
  EXPECT_SUCCEEDS(R"(
    extern func whichString(str: [char]) {
      // @TODO switch expression
      // return switch str {
      //   "no" -> 0.0;
      //   "maybe" -> 0.5;
      //   "yes" -> 1.0;
      //   default -> -1.0;
      // };
    
      switch str {
        case "no" return 0.0;
        case "maybe" return 0.5;
        case "yes" return 1.0;
        default return -1.0;
      }
    }
  )");
  
  auto which = GET_FUNC("whichString", Real(Array<Char>));
  
  EXPECT_EQ(which(makeString("no")), 0.0f);
  EXPECT_EQ(which(makeString("maybe")), 0.5f);
  EXPECT_EQ(which(makeString("yes")), 1.0f);
  EXPECT_EQ(which(makeString("kinda")), -1.0f);
  EXPECT_EQ(which(makeString("")), -1.0f);
}

TEST(Lifetime, Destructors_in_while) {
  EXPECT_SUCCEEDS(R"(
    extern func get_1_ref(val: sint) {
      var array: [real];
      while (val < 10) {
        if (val == -1) {
          let retain = array;
          break;
        }
        let retain0 = array;
        if (val % 2 == 0) {
          val++;
          continue;
        }
        let retain1 = array;
        val *= 2;
      }
      return array;
    }
  )");
  
  auto get = GET_FUNC("get_1_ref", Array<Real>(Sint));
  
  Array<Real> a = get(-1);
  ASSERT_TRUE(a);
  EXPECT_EQ(a.use_count(), 1);
  
  Array<Real> b = get(0);
  ASSERT_TRUE(b);
  EXPECT_EQ(b.use_count(), 1);
  
  Array<Real> c = get(5);
  ASSERT_TRUE(c);
  EXPECT_EQ(c.use_count(), 1);
  
  Array<Real> d = get(10);
  ASSERT_TRUE(d);
  EXPECT_EQ(d.use_count(), 1);
}

TEST(Lifetime, Destructors_in_for) {
  EXPECT_SUCCEEDS(R"(
    extern func get_1_ref(val: sint) {
      var array: [real];
      for (retain0 := array; val < 10; val++) {
        let retain1 = retain0;
        if (val == -1) {
          let retain2 = retain1;
          break;
        }
        if (val % 2 == 0) {
          continue;
        }
        {
          let retain3 = retain1;
        }
        val = val * 2 - 1;
      }
      return array;
    }
  )");
  
  auto get = GET_FUNC("get_1_ref", Array<Real>(Sint));
  
  Array<Real> a = get(-1);
  ASSERT_TRUE(a);
  EXPECT_EQ(a.use_count(), 1);
  
  Array<Real> b = get(0);
  ASSERT_TRUE(b);
  EXPECT_EQ(b.use_count(), 1);
  
  Array<Real> c = get(5);
  ASSERT_TRUE(c);
  EXPECT_EQ(c.use_count(), 1);
  
  Array<Real> d = get(10);
  ASSERT_TRUE(d);
  EXPECT_EQ(d.use_count(), 1);
}

TEST(Func, Modify_local_struct) {
  EXPECT_SUCCEEDS(R"(
    type Struct struct {
      i: sint;
    };
    
    extern func modifyLocal(s: Struct) {
      s.i++;
      return s.i;
    }
    
    extern func modify(s: Struct) {
      return modifyLocal(s);
    }
  )");
  
  struct Struct {
    Sint i;
  };
  
  auto modify = GET_FUNC("modify", Sint(Struct));
  
  Struct s;
  s.i = 11;
  const Sint newVal = modify(s);
  EXPECT_EQ(s.i, 11);
  EXPECT_EQ(newVal, 12);
}

TEST(Lifetime, Pass_array_to_function) {
  EXPECT_SUCCEEDS(R"(
    extern func identityImpl(a: [real]) {
      return a;
    }
    
    extern func identity(a: [real]) {
      return identityImpl(a);
    }
    
    extern func get1_r() {
      return identity(make [real] {});
    }
    
    extern func get1_l() {
      var a: [real];
      return identity(a);
    }
  )");
  
  auto identity = GET_FUNC("identity", Array<Real>(Array<Real>));
  
  Array<Real> a = makeEmptyArray<Real>();
  Array<Real> ret = identity(a);
  ASSERT_TRUE(a);
  ASSERT_TRUE(ret);
  EXPECT_EQ(a, ret);
  EXPECT_EQ(a.use_count(), 2);
  
  EXPECT_EQ(identity(makeEmptyArray<Real>()).use_count(), 1);
  
  auto get1_r = GET_FUNC("get1_r", Array<Real>());
  
  Array<Real> ref1_r = get1_r();
  ASSERT_TRUE(ref1_r);
  EXPECT_EQ(ref1_r.use_count(), 1);
  
  auto get1_l = GET_FUNC("get1_l", Array<Real>());
  
  Array<Real> ref1_l = get1_l();
  ASSERT_TRUE(ref1_l);
  EXPECT_EQ(ref1_l.use_count(), 1);
}

TEST(Lifetime, Pass_struct_to_function) {
  EXPECT_SUCCEEDS(R"(
    type S struct {
      m: [real];
    };
    
    extern func identityImpl(s: S) {
      return s;
    }
    
    extern func identity(s: S) {
      return identityImpl(s);
    }
    
    extern func dontMove(s: ref S) {
      return s;
    }
    
    extern func get1() {
      return identity(make S {});
    }
    
    extern func get1temp() {
      var s: S;
      s = identity(s);
      s = s;
      let t = dontMove(s);
      return s;
    }
  )");
  
  struct S {
    Array<Real> m;
  };
  
  auto identity = GET_FUNC("identity", S(S));
  
  S s = {makeEmptyArray<Real>()};
  S ret = identity(s);
  EXPECT_EQ(s.m.get(), ret.m.get());
  EXPECT_EQ(s.m.use_count(), 2);
  
  S one = identity(S{makeEmptyArray<Real>()});
  ASSERT_TRUE(one.m);
  EXPECT_EQ(one.m.use_count(), 1);
  
  auto get1 = GET_FUNC("get1", S());
  
  S ref1 = get1();
  ASSERT_TRUE(ref1.m);
  EXPECT_EQ(ref1.m.use_count(), 1);
  
  auto get1temp = GET_FUNC("get1temp", S());
  
  S ref1temp = get1temp();
  ASSERT_TRUE(ref1temp.m);
  EXPECT_EQ(ref1temp.m.use_count(), 1);
}

TEST(Lifetime, Copy_elision) {
  EXPECT_SUCCEEDS(R"(
    extern func defaultCtorRet() {
      return make [real] {};
    }
    
    extern func defaultCtorTemp() {
      let temp = defaultCtorRet();
      return temp;
    }
  )");
  
  auto defaultCtorRet = GET_FUNC("defaultCtorRet", Array<Real>());
  
  Array<Real> a = defaultCtorRet();
  ASSERT_TRUE(a);
  EXPECT_EQ(a.use_count(), 1);
  
  auto defaultCtorTemp = GET_FUNC("defaultCtorTemp", Array<Real>());
  
  Array<Real> b = defaultCtorTemp();
  ASSERT_TRUE(b);
  EXPECT_EQ(b.use_count(), 1);
}

TEST(Lifetime, Nontrivial_global_variable) {
  EXPECT_SUCCEEDS(R"(
    var array: [real];
    
    extern func getGlobalArray() {
      return array;
    }
  )");
  
  auto getGlobalArray = GET_FUNC("getGlobalArray", Array<Real>());
  
  Array<Real> a = getGlobalArray();
  ASSERT_TRUE(a);
  EXPECT_EQ(a.use_count(), 2);
}

TEST(Lifetime, Access_materialized_field) {
  EXPECT_SUCCEEDS(R"(
    type Inner struct {
      arr: [real];
    };
    type Outer struct {
      inn: Inner;
    };
    
    extern func returnX() {
      return (make Outer {}).inn.arr;
    }
    
    extern func constructFromX() {
      let temp = (make Outer {}).inn.arr;
      return temp;
    }
    
    extern func assignFromX() {
      var temp: [real];
      temp = (make Outer {}).inn.arr;
      return temp;
    }
    
    extern func constructTempFromX() {
      return make Outer {make Inner {(make Outer {}).inn.arr}};
    }
    
    let global = (make Outer {}).inn.arr;
    
    extern func constructGlobalFromX() {
      return global;
    }
  )");
  
  auto returnX = GET_FUNC("returnX", Array<Real>());
  Array<Real> a = returnX();
  ASSERT_TRUE(a);
  EXPECT_EQ(a.use_count(), 1);
  
  auto constructFromX = GET_FUNC("constructFromX", Array<Real>());
  Array<Real> b = constructFromX();
  ASSERT_TRUE(b);
  EXPECT_EQ(b.use_count(), 1);
  
  auto assignFromX = GET_FUNC("assignFromX", Array<Real>());
  Array<Real> c = assignFromX();
  ASSERT_TRUE(c);
  EXPECT_EQ(c.use_count(), 1);
  
  auto constructTempFromX = GET_FUNC("constructTempFromX", Array<Real>());
  Array<Real> d = constructTempFromX();
  ASSERT_TRUE(d);
  EXPECT_EQ(d.use_count(), 1);
  
  auto constructGlobalFromX = GET_FUNC("constructGlobalFromX", Array<Real>());
  Array<Real> e = constructGlobalFromX();
  ASSERT_TRUE(e);
  EXPECT_EQ(e.use_count(), 2);
}

TEST(Lifetime, Destroy_xvalue_in_empty_switch) {
  EXPECT_SUCCEEDS(R"(
    type S struct {
      a: sint;
      b: [real];
    };
    
    extern func test() {
      switch (make S {}).a {}
      return 0;
    }
  )");
  
  auto test = GET_FUNC("test", Sint());
  EXPECT_EQ(test(), 0);
}

TEST(Compare, Primitives) {
  EXPECT_SUCCEEDS(R"(
    extern func feq(a: real, b: real) {
      return a == b;
    }
    
    extern func ige(a: sint, b: sint) {
      return a >= b;
    }
    
    extern func fle(a: real, b: real) {
      return a <= b;
    }
  )");
  
  auto feq = GET_FUNC("feq", Bool(Real, Real));
  EXPECT_TRUE(feq(0.0f, 0.0f));
  EXPECT_TRUE(feq(2.5f, 2.5f));
  EXPECT_TRUE(feq(1e17f, 1e17f));
  EXPECT_TRUE(feq(1.1f, 1.10f));
  EXPECT_FALSE(feq(0.0f, 1.0f));
  EXPECT_FALSE(feq(1.00001f, 1.00002f));
  
  auto ige = GET_FUNC("ige", Bool(Sint, Sint));
  EXPECT_TRUE(ige(0, 0));
  EXPECT_TRUE(ige(16, 2));
  EXPECT_TRUE(ige(11, 11));
  EXPECT_FALSE(ige(12, 13));
  EXPECT_FALSE(ige(0, 21));
  
  auto fle = GET_FUNC("fle", Bool(Real, Real));
  EXPECT_TRUE(fle(2.5f, 2.5f));
  EXPECT_TRUE(fle(3.0f, 81.0f));
  EXPECT_TRUE(fle(1.00001f, 1.00002f));
  EXPECT_FALSE(fle(1.00002f, 1.00001f));
  EXPECT_FALSE(fle(18.0f, 3.0f));
}

TEST(Expr, Short_circuit_eval) {
  EXPECT_SUCCEEDS(R"(
    var opCount: sint;
  
    extern func getOpCount() {
      return opCount;
    }
    
    func getFalse() {
      opCount++;
      return opCount % 2 * opCount % 2 != opCount % 2;
    }
    func getTrue() {
      opCount++;
      return opCount % 2 * opCount % 2 == opCount % 2;
    }
  
    extern func shortAnd2() {
      return getTrue() && getFalse();
    }
    extern func shortAnd1() {
      return getFalse() && getTrue();
    }
    extern func shortOr2() {
      return getFalse() || getTrue();
    }
    extern func or2() {
      let a = getTrue();
      let b = getFalse();
      return a || b;
    }
  )");
  
  auto op = GET_FUNC("getOpCount", Sint());
  EXPECT_EQ(op(), 0);
  
  auto sand2 = GET_FUNC("shortAnd2", Bool());
  EXPECT_FALSE(sand2());
  EXPECT_EQ(op(), 2);
  
  auto sand1 = GET_FUNC("shortAnd1", Bool());
  EXPECT_FALSE(sand1());
  EXPECT_EQ(op(), 3);
  
  auto sor2 = GET_FUNC("shortOr2", Bool());
  EXPECT_TRUE(sor2());
  EXPECT_EQ(op(), 5);
  
  auto or2 = GET_FUNC("or2", Bool());
  EXPECT_TRUE(or2());
  EXPECT_EQ(op(), 7);
}

TEST(Expr, Array_literal) {
  EXPECT_SUCCEEDS(R"(
    extern func getEmpty() {
      return make [real] [];
    }
    
    extern func oneTwoThree() {
      return [1.0, 2.0, 3.0];
    }
    
    extern func zero() {
      return [0.0, 0.0, 0.0, 0.0];
    }
  )");
  
  auto getEmpty = GET_FUNC("getEmpty", Array<Real>());
  Array<Real> empty = getEmpty();
  ASSERT_TRUE(empty);
  EXPECT_EQ(empty.use_count(), 1);
  EXPECT_EQ(empty->cap, 0);
  EXPECT_EQ(empty->len, 0);
  EXPECT_EQ(empty->dat, nullptr);
  
  auto oneTwoThree = GET_FUNC("oneTwoThree", Array<Real>());
  Array<Real> nums = oneTwoThree();
  ASSERT_TRUE(nums);
  EXPECT_EQ(nums.use_count(), 1);
  EXPECT_EQ(nums->cap, 3);
  EXPECT_EQ(nums->len, 3);
  ASSERT_TRUE(nums->dat);
  EXPECT_EQ(nums->dat[0], 1.0f);
  EXPECT_EQ(nums->dat[1], 2.0f);
  EXPECT_EQ(nums->dat[2], 3.0f);
  
  auto zero = GET_FUNC("zero", Array<Real>());
  Array<Real> zeros = zero();
  ASSERT_TRUE(zeros);
  EXPECT_EQ(zeros.use_count(), 1);
  EXPECT_EQ(zeros->cap, 4);
  EXPECT_EQ(zeros->len, 4);
  ASSERT_TRUE(zeros->dat);
  EXPECT_EQ(zeros->dat[0], 0.0f);
  EXPECT_EQ(zeros->dat[1], 0.0f);
  EXPECT_EQ(zeros->dat[2], 0.0f);
  EXPECT_EQ(zeros->dat[3], 0.0f);
}

TEST(Expr, String_literal) {
  EXPECT_SUCCEEDS(R"(
    extern func getEmpty() {
      return "";
    }
    
    extern func oneTwoThree() {
      return "123";
    }
    
    extern func zero() {
      return "0000";
    }
    
    extern func escapes() {
      return "\'\"\?\\\a\b\f\n\r\t\v\00000000000011\x000011";
    }
  )");
  
  auto getEmpty = GET_FUNC("getEmpty", Array<Char>());
  Array<Char> empty = getEmpty();
  ASSERT_TRUE(empty);
  EXPECT_EQ(empty.use_count(), 1);
  EXPECT_EQ(empty->cap, 0);
  EXPECT_EQ(empty->len, 0);
  EXPECT_EQ(empty->dat, nullptr);
  
  auto oneTwoThree = GET_FUNC("oneTwoThree", Array<Char>());
  Array<Char> nums = oneTwoThree();
  ASSERT_TRUE(nums);
  EXPECT_EQ(nums.use_count(), 1);
  EXPECT_EQ(nums->cap, 3);
  EXPECT_EQ(nums->len, 3);
  ASSERT_TRUE(nums->dat);
  EXPECT_EQ(nums->dat[0], '1');
  EXPECT_EQ(nums->dat[1], '2');
  EXPECT_EQ(nums->dat[2], '3');
  
  auto zero = GET_FUNC("zero", Array<Char>());
  Array<Char> zeros = zero();
  ASSERT_TRUE(zeros);
  EXPECT_EQ(zeros.use_count(), 1);
  EXPECT_EQ(zeros->cap, 4);
  EXPECT_EQ(zeros->len, 4);
  ASSERT_TRUE(zeros->dat);
  EXPECT_EQ(zeros->dat[0], '0');
  EXPECT_EQ(zeros->dat[1], '0');
  EXPECT_EQ(zeros->dat[2], '0');
  EXPECT_EQ(zeros->dat[3], '0');
  
  auto escapes = GET_FUNC("escapes", Array<Char>());
  Array<Char> chars = escapes();
  ASSERT_TRUE(chars);
  EXPECT_EQ(chars.use_count(), 1);
  EXPECT_EQ(chars->cap, 13);
  EXPECT_EQ(chars->len, 13);
  ASSERT_TRUE(chars->dat);
  EXPECT_EQ(chars->dat[0], '\'');
  EXPECT_EQ(chars->dat[1], '\"');
  EXPECT_EQ(chars->dat[2], '?');
  EXPECT_EQ(chars->dat[3], '\\');
  EXPECT_EQ(chars->dat[4], '\a');
  EXPECT_EQ(chars->dat[5], '\b');
  EXPECT_EQ(chars->dat[6], '\f');
  EXPECT_EQ(chars->dat[7], '\n');
  EXPECT_EQ(chars->dat[8], '\r');
  EXPECT_EQ(chars->dat[9], '\t');
  EXPECT_EQ(chars->dat[10], '\v');
  EXPECT_EQ(chars->dat[11], '\11');
  EXPECT_EQ(chars->dat[12], '\x11');
}

TEST(Func, Member_functions) {
  EXPECT_SUCCEEDS(R"(
    extern func (self: sint) plus(other: sint) {
      return self + other;
    }
    
    extern func (self: ref sint) add(other: sint) {
      self += other;
    }
    
    extern func (this: [real]) first() {
      return this[0];
    }
    
    extern func (this: [real]) get() {
      return this;
    }
  )");
  
  auto plus = GET_MEM_FUNC("plus", Sint(Sint, Sint));
  EXPECT_EQ(plus(1, 2), 3);
  EXPECT_EQ(plus(79, -8), 71);
  
  auto add = GET_MEM_FUNC("add", Void(Sint *, Sint));
  Sint sum = 7;
  add(&sum, 7);
  EXPECT_EQ(sum, 14);
  add(&sum, -4);
  EXPECT_EQ(sum, 10);
  
  auto first = GET_MEM_FUNC("first", Real(Array<Real>));
  Array<Real> array = makeArrayOf<Real>(4.0f, 8.0f);
  EXPECT_EQ(first(array), 4.0f);
  array->dat[0] = -0.5f;
  EXPECT_EQ(first(array), -0.5f);
  
  auto get = GET_MEM_FUNC("get", Array<Real>(Array<Real>));
  Array<Real> copy = get(array);
  ASSERT_TRUE(copy);
  EXPECT_EQ(copy, array);
  EXPECT_EQ(copy.use_count(), 2);
}

TEST(Compare, Structs) {
  EXPECT_SUCCEEDS(R"(
    type RS struct {
      a: real;
      b: sint;
    };
    
    extern func eqRS(a: RS, b: RS) {
      return a == b;
    }
    
    type BBBB struct {
      a: byte;
      b: byte;
      c: byte;
      d: byte;
    };
    
    extern func eqBBBB(a: BBBB, b: BBBB) {
      return a == b;
    }
    
    type Str struct {
      a: char;
      b: char;
      c: char;
      d: char;
    };
    
    extern func less(a: Str, b: Str) {
      return a < b;
    }
  )");
  
  struct RS {
    Real a;
    Sint b;
  };
  
  auto eqRS = GET_FUNC("eqRS", Bool(RS, RS));
  EXPECT_TRUE(eqRS(RS{0.0f, 0}, RS{0.0f, 0}));
  EXPECT_TRUE(eqRS(RS{5.5f, 5}, RS{5.5f, 5}));
  EXPECT_FALSE(eqRS(RS{0.1f, 0}, RS{0.0f, 0}));
  EXPECT_FALSE(eqRS(RS{0.0f, 0}, RS{0.0f, 1}));
  
  struct BBBB {
    Byte a, b, c, d;
  };
  
  auto eqBBBB = GET_FUNC("eqBBBB", Bool(BBBB, BBBB));
  EXPECT_TRUE(eqBBBB(BBBB{0, 0, 0, 0}, BBBB{0, 0, 0, 0}));
  EXPECT_TRUE(eqBBBB(BBBB{1, 2, 3, 4}, BBBB{1, 2, 3, 4}));
  EXPECT_FALSE(eqBBBB(BBBB{4, 3, 2, 1}, BBBB{3, 4, 1, 2}));
  EXPECT_FALSE(eqBBBB(BBBB{1, 2, 3, 4}, BBBB{4, 3, 2, 1}));
  
  struct Str {
    Char a, b, c, d;
  };
  
  auto less = GET_FUNC("less", Bool(Str, Str));
  EXPECT_TRUE(less(Str{'a', 'b', 'c', 'd'}, Str{'b', 'c', 'd', 'e'}));
  EXPECT_TRUE(less(Str{'a', 'b', 'c', 'd'}, Str{'a', 'b', 'c', 'e'}));
  EXPECT_FALSE(less(Str{'a', 'b', 'c', 'd'}, Str{'a', 'b', 'c', 'd'}));
  EXPECT_FALSE(less(Str{'b', 'b', 'c', 'd'}, Str{'a', 'b', 'c', 'd'}));
}

TEST(Expr, Array_of_arrays) {
  EXPECT_SUCCEEDS(R"(
    extern func wrap(inner: [real]) {
      let ret = [inner];
      return ret;
    }
    
    extern func unwrap(outer: [[real]]) {
      let dup = outer;
      return dup[0];
    }
  )");
  
  auto wrap = GET_FUNC("wrap", Array<Array<Real>>(Array<Real>));
  
  Array<Real> inner = makeEmptyArray<Real>();
  EXPECT_EQ(inner.use_count(), 1);
  {
    Array<Array<Real>> wrapped = wrap(inner);
    ASSERT_TRUE(wrapped);
    EXPECT_EQ(wrapped.use_count(), 1);
    ASSERT_TRUE(inner);
    EXPECT_EQ(inner.use_count(), 2);
    EXPECT_EQ(wrapped->len, 1);
    ASSERT_TRUE(wrapped->dat);
    EXPECT_EQ(inner, wrapped->dat[0]);
  }
  EXPECT_EQ(inner.use_count(), 1);
  
  auto unwrap = GET_FUNC("unwrap", Array<Real>(Array<Array<Real>>));
  
  {
    Array<Array<Real>> wrapped = wrap(inner);
    EXPECT_EQ(inner.use_count(), 2);
    {
      Array<Real> innerCopy = unwrap(wrapped);
      EXPECT_EQ(wrapped.use_count(), 1);
      ASSERT_TRUE(innerCopy);
      EXPECT_EQ(innerCopy, inner);
      EXPECT_EQ(inner.use_count(), 3);
    }
    EXPECT_EQ(inner.use_count(), 2);
  }
  EXPECT_EQ(inner.use_count(), 1);
}

TEST(Compare, Arrays) {
  EXPECT_SUCCEEDS(R"(
    extern func less(a: [char], b: [char]) {
      return a < b;
    }
    
    extern func eq(a: [char], b: [char]) {
      return a == b;
    }
  )");
  
  auto less = GET_FUNC("less", Bool(Array<Char>, Array<Char>));
  
  EXPECT_TRUE(less(makeString("abc"), makeString("abcd")));
  EXPECT_TRUE(less(makeString("abcd"), makeString("abce")));
  EXPECT_TRUE(less(makeString("a"), makeString("b")));
  EXPECT_TRUE(less(makeString(""), makeString("a")));
  EXPECT_FALSE(less(makeString(""), makeString("")));
  EXPECT_FALSE(less(makeEmptyArray<Char>(), makeEmptyArray<Char>()));
  EXPECT_FALSE(less(makeString("abc"), makeString("abc")));
  EXPECT_FALSE(less(makeString("abce"), makeString("abcd")));
  EXPECT_FALSE(less(makeString("abcd"), makeString("abc")));
  
  auto eq = GET_FUNC("eq", Bool(Array<Char>, Array<Char>));
  
  EXPECT_TRUE(eq(makeString("abc"), makeString("abc")));
  EXPECT_TRUE(eq(makeString("a"), makeString("a")));
  EXPECT_TRUE(eq(makeString(""), makeString("")));
  EXPECT_TRUE(eq(makeEmptyArray<Char>(), makeEmptyArray<Char>()));
  EXPECT_FALSE(eq(makeString("a"), makeString("b")));
  EXPECT_FALSE(eq(makeString("abcdefg"), makeString("abcdeg")));
  EXPECT_FALSE(eq(makeString("abcd"), makeString("dcba")));
  EXPECT_FALSE(eq(makeString("abc"), makeString("abcd")));
  EXPECT_FALSE(eq(makeString("abcd"), makeString("abc")));
}

TEST(Compare, Sort_strings) {
  EXPECT_SUCCEEDS(R"(
    func swap(a: ref [char], b: ref [char]) {
      let t = a;
      a = b;
      b = t;
    }
    
    extern func bubbles(arr: [[char]], len: sint) {
      if (len < 2) return;
      let numPairs = len - 1;
      var sorted = false;
      while (!sorted) {
        sorted = true;
        for (p := 0; p != numPairs; p++) {
          if (arr[p] > arr[p + 1]) {
            swap(arr[p], arr[p + 1]);
            sorted = false;
          }
        }
      }
    }
  )");
  
  auto sort = GET_FUNC("bubbles", Void(Array<Array<Char>>, Sint));
  
  Array<Char> stat = makeString("statically");
  Array<Char> type = makeString("typed");
  Array<Char> embd = makeString("embeddable");
  Array<Char> lang = makeString("language");
  Array<Array<Char>> arr = makeArrayOf<Array<Char>>(stat, type, embd, lang);
  EXPECT_EQ(stat.use_count(), 2);
  EXPECT_EQ(type.use_count(), 2);
  EXPECT_EQ(embd.use_count(), 2);
  EXPECT_EQ(lang.use_count(), 2);
  EXPECT_EQ(arr.use_count(), 1);
  
  sort(arr, arr->len);
  
  EXPECT_EQ(stat.use_count(), 2);
  EXPECT_EQ(type.use_count(), 2);
  EXPECT_EQ(embd.use_count(), 2);
  EXPECT_EQ(lang.use_count(), 2);
  EXPECT_EQ(arr.use_count(), 1);
  EXPECT_EQ(arr->dat[0], embd);
  EXPECT_EQ(arr->dat[1], lang);
  EXPECT_EQ(arr->dat[2], stat);
  EXPECT_EQ(arr->dat[3], type);
}

TEST(Btn_func, capacity_size) {
  EXPECT_SUCCEEDS(R"(
    extern func getCap(arr: ref [real]) {
      return capacity(arr);
    }
    extern func getLen(arr: ref [real]) {
      return size(arr);
    }
    extern func getTwelve() {
      return size("Hello World!");
    }
  )");
  
  auto getCap = GET_FUNC("getCap", Uint(Array<Real> &));
  auto getLen = GET_FUNC("getLen", Uint(Array<Real> &));
  
  Array<Real> array = makeEmptyArray<Real>();
  
  EXPECT_EQ(getCap(array), 0);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(getLen(array), 0);
  EXPECT_EQ(array.use_count(), 1);
  
  array->cap = 789;
  array->len = 987;
  
  EXPECT_EQ(getCap(array), 789);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(getLen(array), 987);
  EXPECT_EQ(array.use_count(), 1);
  
  auto getTwelve = GET_FUNC("getTwelve", Uint());
  
  EXPECT_EQ(getTwelve(), 12);
  EXPECT_EQ(getTwelve(), 12);
}

TEST(Btn_func, pop_back) {
  EXPECT_SUCCEEDS(R"(
    extern func pop(arr: ref [[char]]) {
      pop_back(arr);
    }
  )");
  
  auto pop = GET_FUNC("pop", Void(Array<Array<Char>> &));
  
  Array<Char> str = makeString("String");
  EXPECT_EQ(str.use_count(), 1);
  Array<Array<Char>> array = makeArrayOf<Array<Char>>(str);
  EXPECT_EQ(str.use_count(), 2);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->len, 1);
  
  pop(array);
  
  EXPECT_EQ(str.use_count(), 1);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->len, 0);
}

TEST(Btn_func, reserve) {
  EXPECT_SUCCEEDS(R"(
    extern func res(arr: ref [[char]], cap: uint) {
      reserve(arr, cap);
    }
  )");
  
  auto res = GET_FUNC("res", Void(Array<Array<Char>> &, Uint));
  
  Array<Char> str = makeString("String");
  EXPECT_EQ(str.use_count(), 1);
  Array<Array<Char>> array = makeArrayOf<Array<Char>>(str);
  EXPECT_EQ(str.use_count(), 2);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->len, 1);
  EXPECT_EQ(array->cap, 1);
  
  res(array, 0);
  
  EXPECT_EQ(str.use_count(), 2);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->len, 1);
  EXPECT_EQ(array->cap, 1);
  
  res(array, 1);
  
  EXPECT_EQ(str.use_count(), 2);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->len, 1);
  EXPECT_EQ(array->cap, 1);
  
  res(array, 2);
  
  EXPECT_EQ(str.use_count(), 2);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->len, 1);
  EXPECT_EQ(array->cap, 2);
  
  res(array, 8);
  
  EXPECT_EQ(str.use_count(), 2);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->len, 1);
  EXPECT_EQ(array->cap, 8);
  
  res(array, 4);
  
  EXPECT_EQ(str.use_count(), 2);
  EXPECT_EQ(array.use_count(), 1);
  EXPECT_EQ(array->len, 1);
  EXPECT_EQ(array->cap, 8);
  
  Array<Array<Char>> empty = makeEmptyArray<Array<Char>>();
  EXPECT_EQ(empty.use_count(), 1);
  EXPECT_EQ(empty->len, 0);
  EXPECT_EQ(empty->cap, 0);
  
  res(empty, 4);
  
  EXPECT_EQ(empty.use_count(), 1);
  EXPECT_EQ(empty->len, 0);
  EXPECT_EQ(empty->cap, 4);
}

TEST(Btn_func, append) {
  EXPECT_SUCCEEDS(R"(
    extern func app(arr: ref [[char]], other: ref [[char]]) {
      append(arr, other);
    }
  )");
  
  auto app = GET_FUNC("app", Void(Array<Array<Char>> &, Array<Array<Char>> &));
  
  Array<Char> str0 = makeString("String");
  Array<Array<Char>> arr0 = makeArrayOf<Array<Char>>(str0);
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(arr0->len, 1);
  EXPECT_EQ(arr0->cap, 1);
  
  Array<Char> str1 = makeString("in");
  Array<Array<Char>> arr1 = makeArrayOf<Array<Char>>(str1);
  EXPECT_EQ(str1.use_count(), 2);
  EXPECT_EQ(arr1->len, 1);
  EXPECT_EQ(arr1->cap, 1);
  
  // ["String"] += ["in"]
  app(arr0, arr1);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(arr1.use_count(), 1);
  EXPECT_EQ(str1.use_count(), 3);
  EXPECT_EQ(arr0->len, 2);
  EXPECT_EQ(arr0->cap, 2);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(arr0->dat[1], str1);
  
  Array<Char> str2 = makeString("an");
  Array<Char> str3 = makeString("array");
  Array<Char> str4 = makeString("of");
  Array<Char> str5 = makeString("strings");
  Array<Array<Char>> arr2 = makeArrayOf<Array<Char>>(str2, str3, str4, str5);
  EXPECT_EQ(str2.use_count(), 2);
  EXPECT_EQ(str3.use_count(), 2);
  EXPECT_EQ(str4.use_count(), 2);
  EXPECT_EQ(str5.use_count(), 2);
  EXPECT_EQ(arr2->len, 4);
  EXPECT_EQ(arr2->cap, 4);
  
  // ["String", "in"] += ["an", "array", "of", "strings"]
  app(arr0, arr2);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(arr1.use_count(), 1);
  EXPECT_EQ(str1.use_count(), 3);
  EXPECT_EQ(arr2.use_count(), 1);
  EXPECT_EQ(str2.use_count(), 3);
  EXPECT_EQ(str3.use_count(), 3);
  EXPECT_EQ(str4.use_count(), 3);
  EXPECT_EQ(str5.use_count(), 3);
  EXPECT_EQ(arr0->len, 6);
  EXPECT_EQ(arr0->cap, 8);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(arr0->dat[1], str1);
  EXPECT_EQ(arr0->dat[2], str2);
  EXPECT_EQ(arr0->dat[3], str3);
  EXPECT_EQ(arr0->dat[4], str4);
  EXPECT_EQ(arr0->dat[5], str5);
  
  Array<Array<Char>> arr3 = makeEmptyArray<Array<Char>>();
  EXPECT_EQ(arr3->len, 0);
  EXPECT_EQ(arr3->cap, 0);
  
  // ["String", "in", "an", "array", "of", "strings"] += []
  app(arr0, arr3);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(arr1.use_count(), 1);
  EXPECT_EQ(str1.use_count(), 3);
  EXPECT_EQ(arr2.use_count(), 1);
  EXPECT_EQ(str2.use_count(), 3);
  EXPECT_EQ(str3.use_count(), 3);
  EXPECT_EQ(str4.use_count(), 3);
  EXPECT_EQ(str5.use_count(), 3);
  EXPECT_EQ(arr3.use_count(), 1);
  EXPECT_EQ(arr0->len, 6);
  EXPECT_EQ(arr0->cap, 8);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(arr0->dat[1], str1);
  EXPECT_EQ(arr0->dat[2], str2);
  EXPECT_EQ(arr0->dat[3], str3);
  EXPECT_EQ(arr0->dat[4], str4);
  EXPECT_EQ(arr0->dat[5], str5);
  
  // ["String", "in", "an", "array", "of", "strings"] += ["String", "in", "an", "array", "of", "strings"]
  app(arr0, arr0);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 3);
  EXPECT_EQ(str1.use_count(), 4);
  EXPECT_EQ(str2.use_count(), 4);
  EXPECT_EQ(str3.use_count(), 4);
  EXPECT_EQ(str4.use_count(), 4);
  EXPECT_EQ(str5.use_count(), 4);
  EXPECT_EQ(arr0->len, 12);
  EXPECT_EQ(arr0->cap, 16);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(arr0->dat[1], str1);
  EXPECT_EQ(arr0->dat[2], str2);
  EXPECT_EQ(arr0->dat[3], str3);
  EXPECT_EQ(arr0->dat[4], str4);
  EXPECT_EQ(arr0->dat[5], str5);
  EXPECT_EQ(arr0->dat[6], str0);
  EXPECT_EQ(arr0->dat[7], str1);
  EXPECT_EQ(arr0->dat[8], str2);
  EXPECT_EQ(arr0->dat[9], str3);
  EXPECT_EQ(arr0->dat[10], str4);
  EXPECT_EQ(arr0->dat[11], str5);
  
  // [] += []
  app(arr3, arr3);
  
  EXPECT_EQ(arr3.use_count(), 1);
  EXPECT_EQ(arr3->len, 0);
  EXPECT_EQ(arr3->cap, 0);
  
  // [] += ["String", "in", "an", "array", "of", "strings", "String", "in", "an", "array", "of", "strings"]
  app(arr3, arr0);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 5);
  EXPECT_EQ(str1.use_count(), 6);
  EXPECT_EQ(str2.use_count(), 6);
  EXPECT_EQ(str3.use_count(), 6);
  EXPECT_EQ(str4.use_count(), 6);
  EXPECT_EQ(str5.use_count(), 6);
  EXPECT_EQ(arr3->len, 12);
  EXPECT_EQ(arr3->cap, 16);
  EXPECT_EQ(arr3->dat[0], str0);
  EXPECT_EQ(arr3->dat[1], str1);
  EXPECT_EQ(arr3->dat[2], str2);
  EXPECT_EQ(arr3->dat[3], str3);
  EXPECT_EQ(arr3->dat[4], str4);
  EXPECT_EQ(arr3->dat[5], str5);
  EXPECT_EQ(arr3->dat[6], str0);
  EXPECT_EQ(arr3->dat[7], str1);
  EXPECT_EQ(arr3->dat[8], str2);
  EXPECT_EQ(arr3->dat[9], str3);
  EXPECT_EQ(arr3->dat[10], str4);
  EXPECT_EQ(arr3->dat[11], str5);
}

TEST(Btn_func, push_back) {
  EXPECT_SUCCEEDS(R"(
    extern func pushStr(arr: ref [[char]], val: [char]) {
      push_back(arr, val);
    }
    
    extern func pushChr(arr: ref [char], val: char) {
      push_back(arr, val);
    }
  )");
  
  auto pushStr = GET_FUNC("pushStr", Void(Array<Array<Char>> &, Array<Char>));
  auto pushChr = GET_FUNC("pushChr", Void(Array<Char> &, Char));
  
  Array<Char> str0 = makeString("String");
  Array<Array<Char>> arr0 = makeArrayOf<Array<Char>>(str0);
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(arr0->len, 1);
  EXPECT_EQ(arr0->cap, 1);
  
  Array<Char> str1 = makeString("array");
  
  pushStr(arr0, str1);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(str1.use_count(), 2);
  EXPECT_EQ(arr0->len, 2);
  EXPECT_EQ(arr0->cap, 2);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(arr0->dat[1], str1);
  
  pushStr(arr0, str1);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(str1.use_count(), 3);
  EXPECT_EQ(arr0->len, 3);
  EXPECT_EQ(arr0->cap, 4);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(arr0->dat[1], str1);
  EXPECT_EQ(arr0->dat[2], str1);
  
  pushStr(arr0, str1);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(str1.use_count(), 4);
  EXPECT_EQ(arr0->len, 4);
  EXPECT_EQ(arr0->cap, 4);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(arr0->dat[1], str1);
  EXPECT_EQ(arr0->dat[2], str1);
  EXPECT_EQ(arr0->dat[3], str1);
  
  Array<Array<Char>> arr1 = makeEmptyArray<Array<Char>>();
  
  pushStr(arr1, str0);
  
  EXPECT_EQ(arr1.use_count(), 1);
  EXPECT_EQ(str0.use_count(), 3);
  EXPECT_EQ(arr1->len, 1);
  EXPECT_EQ(arr1->cap, 1);
  EXPECT_EQ(arr1->dat[0], str0);
  
  pushChr(str0, 'y');
  
  EXPECT_EQ(str0.use_count(), 3);
  EXPECT_EQ(str0->len, 7);
  EXPECT_EQ(str0->cap, 8);
  EXPECT_EQ(str0->dat[0], 'S');
  EXPECT_EQ(str0->dat[1], 't');
  EXPECT_EQ(str0->dat[2], 'r');
  EXPECT_EQ(str0->dat[3], 'i');
  EXPECT_EQ(str0->dat[4], 'n');
  EXPECT_EQ(str0->dat[5], 'g');
  EXPECT_EQ(str0->dat[6], 'y');
}

TEST(Btn_func, resize) {
  EXPECT_SUCCEEDS(R"(
    extern func resStr(arr: ref [[char]], len: uint) {
      resize(arr, len);
    }
    
    extern func resChr(arr: ref [char], len: uint) {
      resize(arr, len);
    }
  )");
  
  auto resStr = GET_FUNC("resStr", Void(Array<Array<Char>> &, Uint));
  auto resChr = GET_FUNC("resChr", Void(Array<Char> &, Uint));
  
  Array<Array<Char>> arr0 = makeEmptyArray<Array<Char>>();
  
  resStr(arr0, 0);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(arr0->len, 0);
  EXPECT_EQ(arr0->cap, 0);
  EXPECT_EQ(arr0->dat, nullptr);
  
  resStr(arr0, 1);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(arr0->len, 1);
  EXPECT_EQ(arr0->cap, 1);
  EXPECT_EQ(arr0->dat[0].use_count(), 1);
  
  Array<Char> str0 = arr0->dat[0];
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(str0->len, 0);
  EXPECT_EQ(str0->cap, 0);
  EXPECT_EQ(str0->dat, nullptr);
  
  resStr(arr0, 2);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(arr0->len, 2);
  EXPECT_EQ(arr0->cap, 2);
  EXPECT_EQ(arr0->dat[0].use_count(), 2);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(arr0->dat[1].use_count(), 1);
  
  Array<Char> str1 = arr0->dat[1];
  EXPECT_EQ(str1.use_count(), 2);
  EXPECT_EQ(str1->len, 0);
  EXPECT_EQ(str1->cap, 0);
  EXPECT_EQ(str1->dat, nullptr);
  
  resStr(arr0, 1);
  
  EXPECT_EQ(arr0.use_count(), 1);
  EXPECT_EQ(arr0->len, 1);
  EXPECT_EQ(arr0->cap, 2);
  EXPECT_EQ(arr0->dat[0].use_count(), 2);
  EXPECT_EQ(arr0->dat[0], str0);
  EXPECT_EQ(str1.use_count(), 1);
  
  resChr(str0, 5);
  
  EXPECT_EQ(str0.use_count(), 2);
  EXPECT_EQ(str0->len, 5);
  EXPECT_EQ(str0->cap, 8);
  EXPECT_EQ(str0->dat[0], 0);
  EXPECT_EQ(str0->dat[1], 0);
  EXPECT_EQ(str0->dat[2], 0);
  EXPECT_EQ(str0->dat[3], 0);
  EXPECT_EQ(str0->dat[4], 0);
}

template <typename Type>
Type identity(Type obj) noexcept {
  return obj;
}

TEST(Closure, Pass_closure) {
  EXPECT_SUCCEEDS(R"(
    type Closure = func(struct {}) -> struct {};
  
    func passClosureImpl(clo: Closure) {
      return clo;
    }
    
    extern func passClosure(clo: Closure) {
      return passClosureImpl(clo);
    }
    
    extern func getStub() {
      let stub = make Closure {};
      return passClosure(stub);
    }
  )");
  
  struct Empty {};
  using ClosureType = Closure<Empty(Empty)>;
  
  auto passClosure = GET_FUNC("passClosure", ClosureType(ClosureType));
  auto getStub = GET_FUNC("getStub", ClosureType());
  
  Closure<Empty(Empty)> stub = getStub();
  
  Closure<Empty(Empty)> closure = makeClosureFromFunc<identity<Empty>>();
  Closure<Empty(Empty)> sameClosure = passClosure(closure);
  
  EXPECT_EQ(closure.fun, sameClosure.fun);
  EXPECT_EQ(closure.dat, sameClosure.dat);
}

TEST(Closure, Function_pointers) {
  EXPECT_SUCCEEDS(R"(
    extern func fn(first: sint, second: real) -> bool {
      return (make real first) == second;
    }
    
    extern func fols() {
      var def: func(sint, ref real) -> bool;
      return def ? true : false;
    }
    
    extern func troo() {
      let ptr = fn;
      if (ptr) {
        return ptr(4, 4.0);
      }
      return false;
    }
  )");
  
  auto fols = GET_FUNC("fols", Bool());
  EXPECT_FALSE(fols());
  
  auto troo = GET_FUNC("troo", Bool());
  EXPECT_TRUE(troo());
}

TEST(Closure, Lambda) {
  EXPECT_SUCCEEDS(R"(
    extern func doNothing() {
      let empty = func(){};
      empty();
    }
  
    extern func three() {
      let add = func(a: sint, b: sint) {
        return a + b;
      };
      return add(1, 2);
    }
  )");
  
  auto nothing = GET_FUNC("doNothing", Void());
  nothing();
  
  auto three = GET_FUNC("three", Sint());
  EXPECT_EQ(three(), 3);
}

TEST(Closure, Stateful_lambda) {
  EXPECT_SUCCEEDS(R"(
    func makeIDgen(first: sint) {
      return func() {
        let id = first;
        first++;
        return id;
      };
    }

    extern func getProduct() {
      var product = 1;
      let gen = makeIDgen(4);
      product *= gen(); // 4
      product *= gen(); // 5
      product *= gen(); // 6
      let otherGen = gen;
      product *= otherGen(); // 7
      product *= gen(); // 8
      product *= otherGen(); // 9
      return product;
    }
  )");
  
  auto getProduct = GET_FUNC("getProduct", Sint());
  EXPECT_EQ(getProduct(), 4 * 5 * 6 * 7 * 8 * 9);
}

TEST(Closure, Immediately_invoked_lambda) {
  EXPECT_SUCCEEDS(R"(
    extern func getNine() {
      return func(){
        return 4 + 5;
      }();
    }
  )");
  
  auto getNine = GET_FUNC("getNine", Sint());
  EXPECT_EQ(getNine(), 9);
}

TEST(Closure, Nested_lambda) {
  EXPECT_SUCCEEDS(R"(
    func makeAdd(a: char) {
      // lam_0: a - p_1
      // cap a has no index
      return func(b: real) {
        // lam_1: a - c_0, b - p_1
        // cap a has index 0
        // cap b has no index
        return func(c: sint) {
          // c_0 + c_1 + p_1
          // a has index 0
          // b has index 1
          // c has no index
          return (make sint a) + (make sint b) + c;
        };
      };
    }
    
    extern func six() {
      let add_1 = makeAdd(1c);
      let add_1_2 = add_1(2.0);
      return add_1_2(3);
    }
  )");
  
  auto six = GET_FUNC("six", Sint());
  EXPECT_EQ(six(), 6);
}

TEST(Closure, Nested_nested_lambda) {
  EXPECT_SUCCEEDS(R"(
    func makeAdd(a: sint) {
      // lam_0: a - p_1
      // cap a has no index
      var other0 = 0;
      return func(b: struct { m: uint; }) {
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
            b.m *= 2u;
            other0++;
            // c_0 + c_1 + c_2 + p_1
            // a has index 0
            // b has index 1
            // c has index 2
            // d has no index
            let sum = make real a +
                      make real b.m +
                      make real c +
                      make real d;
            // sum is 20.0
            // other0 is 3
            // other1 is 4
            // other2 is 3
            return sum *
                   make real other0 *
                   make real other1 *
                   make real other2;
          };
        };
      };
    }
    
    extern func get() {
      let add_1 = makeAdd(1);
      let add_1_2 = add_1(make struct { m: uint; } {2u});
      let add_1_2_3 = add_1_2(3b);
      return add_1_2_3(4c);
    }
  )");
  
  auto get = GET_FUNC("get", Real());
  EXPECT_EQ(get(), 20.0f * 3.0f * 4.0f * 3.0f);
}

TEST(Btn_func, Squares_array) {
  EXPECT_SUCCEEDS(R"(
    extern func squares(count: uint) -> [uint] {
      var array: [uint];
      reserve(array, count);
      for (i := 0u; i < count; i++) {
        push_back(array, i * i);
      }
      return array;
    }

    extern func zero3() {
      return [[[0.0]]][0][0][0];
    }
    
    extern func zero2() {
      return [[0.0]][0][0];
    }
    
    extern func zero1() {
      return [0.0][0];
    }
  )");
  
  auto squares = GET_FUNC("squares", Array<Uint>(Uint));
  
  Array<Uint> empty = squares(0);
  ASSERT_TRUE(empty);
  EXPECT_EQ(empty.use_count(), 1);
  EXPECT_EQ(empty->len, 0);
  
  Array<Uint> zero_one_four_nine = squares(4);
  ASSERT_TRUE(zero_one_four_nine);
  EXPECT_EQ(zero_one_four_nine.use_count(), 1);
  EXPECT_EQ(zero_one_four_nine->len, 4);
  EXPECT_EQ(zero_one_four_nine->cap, 4);
  ASSERT_TRUE(zero_one_four_nine->dat);
  EXPECT_EQ(zero_one_four_nine->dat[0], 0.0f);
  EXPECT_EQ(zero_one_four_nine->dat[1], 1.0f);
  EXPECT_EQ(zero_one_four_nine->dat[2], 4.0f);
  EXPECT_EQ(zero_one_four_nine->dat[3], 9.0f);
  
  const Uint lots = 1000;
  
  Array<Uint> lotsSquares = squares(lots);
  ASSERT_TRUE(lotsSquares);
  EXPECT_EQ(lotsSquares.use_count(), 1);
  EXPECT_EQ(lotsSquares->len, lots);
  EXPECT_EQ(lotsSquares->cap, lots);
  ASSERT_TRUE(lotsSquares->dat);
  
  for (Uint n = 0; n != lots; ++n) {
    EXPECT_EQ(lotsSquares->dat[n], n * n);
  }
  
  // @TODO investigate malloc optimization
  auto zero3 = GET_FUNC("zero3", Real());
  EXPECT_EQ(zero3(), 0.0f);
  
  auto zero2 = GET_FUNC("zero2", Real());
  EXPECT_EQ(zero2(), 0.0f);
  
  auto zero1 = GET_FUNC("zero1", Real());
  EXPECT_EQ(zero1(), 0.0f);
}

TEST(Basic, Modules) {
  const char *sourceA = R"(
    module glm;

    type vec2 struct {
      x: real;
      y: real;
    };

    func add(a: vec2, b: vec2) {
      return make vec2 {a.x + b.x, a.y + b.y};
    }

    // We don't have a builtin sqrt function yet so this will have to do
    func (v: vec2) mag2() {
      return v.x * v.x + v.y * v.y;
    }
  )";
  
  const char *sourceB = R"(
    // Optional
    // module main;

    import glm;

    extern func five() {
      let one_two = make vec2 {1.0, 2.0};
      let three_four = make vec2 {3.0, 4.0};
      let four_six = add(one_two, three_four);
      return one_two.mag2();
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(sourceB, log()));
  asts.push_back(createAST(sourceA, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  
  auto *engine = generate(syms, log());
  
  auto five = GET_FUNC("five", Real());
  EXPECT_EQ(five(), 5.0f);
}

TEST(Closure, Bool_conv) {
  EXPECT_SUCCEEDS(R"(
    extern func passed() {
      var nullFn: func();
      if (nullFn) return false;
      for (;nullFn;) return false;
      while (nullFn) return false;
      let two = nullFn ? 6 : 2;
      if (two != 2) return false;
      let troo = !make bool nullFn;
      if (!troo) return false;
      let tru = !nullFn;
      if (!tru) return false;
      let fls = true && nullFn;
      if (fls) return false;
      let fols = false || nullFn;
      if (fols) return false;
      let fals: bool = nullFn;
      if (fals) return false;
      return true;
    }
  )");
  
  auto passed = GET_FUNC("passed", Bool());
  EXPECT_TRUE(passed());
}

TEST(Closure, Lambda_bool_conv) {
  EXPECT_SUCCEEDS(R"(
    extern func passed() {
      return make bool func(){} && !!(func(){});
    }
  )");
  
  auto passed = GET_FUNC("passed", Bool());
  EXPECT_TRUE(passed());
}

TEST(Closure, Return_function) {
  EXPECT_SUCCEEDS(R"(
    extern func five() {
      return 5;
    }
    
    extern func getFive() {
      return five;
    }
  )");
  
  auto getFive = GET_FUNC("getFive", Closure<Sint()>());
  auto five = getFive();
  EXPECT_EQ(five(), 5);
}

TEST(Closure, Return_lambda) {
  EXPECT_SUCCEEDS(R"(
    extern func getFive() {
      return func() {
        return 5;
      };
    }
  )");
  
  auto getFive = GET_FUNC("getFive", Closure<Sint()>());
  auto five = getFive();
  ASSERT_TRUE(five.dat);
  EXPECT_EQ(five.dat.use_count(), 1);
  EXPECT_EQ(five(), 5);
}

TEST(Expr, Compound_operator) {
  EXPECT_SUCCEEDS(R"(
    extern func fac(param: sint) {
      var ret = 1;
      for (i := 1; i <= param; i++) {
        ret *= i;
      }
      return ret;
    }
  )");
  
  auto fac = GET_FUNC("fac", Sint(Sint));
  EXPECT_EQ(fac(0), 1);
  EXPECT_EQ(fac(1), 1);
  EXPECT_EQ(fac(2), 2);
  EXPECT_EQ(fac(3), 6);
  EXPECT_EQ(fac(4), 24);
}

TEST(Loops, All) {
  EXPECT_SUCCEEDS(R"(
    extern func breaks(param: sint) {
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
    
    extern func continues(param: sint) {
      var ret = 1;
      
      for (i := param; i > 0; i--) {
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
  
  auto breaks = GET_FUNC("breaks", Sint(Sint));
  
  EXPECT_EQ(breaks(-6), 0);
  EXPECT_EQ(breaks(-5), 0);
  EXPECT_EQ(breaks(-4), 0);
  EXPECT_EQ(breaks(-3), 0);
  EXPECT_EQ(breaks(-2), 0);
  EXPECT_EQ(breaks(-1), 0);
  EXPECT_EQ(breaks(0), 0);
  EXPECT_EQ(breaks(1), 1);
  EXPECT_EQ(breaks(2), 3);
  EXPECT_EQ(breaks(3), 12);
  EXPECT_EQ(breaks(4), 12);
  EXPECT_EQ(breaks(5), 12);
  EXPECT_EQ(breaks(6), 12);
  
  auto continues = GET_FUNC("continues", Sint(Sint));
  
  EXPECT_EQ(continues(-6), 1);
  EXPECT_EQ(continues(-5), 1);
  EXPECT_EQ(continues(-4), 1);
  EXPECT_EQ(continues(-3), 1);
  EXPECT_EQ(continues(-2), 1);
  EXPECT_EQ(continues(-1), 1);
  EXPECT_EQ(continues(0), 1);
  EXPECT_EQ(continues(1), 1);
  EXPECT_EQ(continues(2), 2);
  EXPECT_EQ(continues(3), 6);
  EXPECT_EQ(continues(4), 9);
  EXPECT_EQ(continues(5), 9);
  EXPECT_EQ(continues(6), 9);
}

TEST(Closure, No_move_return_captures) {
  EXPECT_SUCCEEDS(R"(
    extern func getClosure(arr: [real]) {
      return func() {
        return arr;
      };
    }
  )");
  
  auto getClosure = GET_FUNC("getClosure", Closure<Array<Real>()>(Array<Real>));
  
  Array<Real> array = makeEmptyArray<Real>();
  EXPECT_EQ(array.use_count(), 1);
  auto closure = getClosure(array);
  EXPECT_EQ(array.use_count(), 2);
  Array<Real> arrayCop = closure();
  EXPECT_EQ(array.use_count(), 3);
  EXPECT_EQ(arrayCop, array);
  Array<Real> arrayCop1 = closure();
  EXPECT_EQ(array.use_count(), 4);
  EXPECT_EQ(arrayCop1, array);
}

retain_ptr<ast::ExtFunc> makeSqrt(sym::Builtins &btn) {
  auto sqrt = make_retain<ast::ExtFunc>();
  sqrt->name = "sqrt";
  sqrt->mangledName = "sqrtf";
  sqrt->params.push_back({ast::ParamRef::val, btn.Real});
  sqrt->ret = btn.Real;
  return sqrt;
}

AST makeCmath(sym::Builtins &btn) {
  AST cmath;
  cmath.name = "cmath";
  cmath.global.push_back(makeSqrt(btn));
  return cmath;
}

TEST(External_Func, Basic) {
  const char *source = R"(
    import cmath;
  
    type Vec2 struct {
      x: real;
      y: real;
    };
  
    func mag(v: Vec2) -> real {
      return sqrt(v.x*v.x + v.y*v.y);
    }
  
    extern func five() {
      return mag(make Vec2 {3.0, 4.0});
    }
  )";

  Symbols syms = initModules(log());
  ASTs asts;
  
  asts.push_back(createAST(source, log()));
  asts.push_back(makeCmath(syms.builtins));

  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  llvm::ExecutionEngine *engine = generate(syms, log());
  
  auto five = GET_FUNC("five", Real());
  EXPECT_EQ(five(), 5.0f);
}

} // namespace

template <>
struct stela::reflect<FILE *> {
  STELA_REFLECT_NAME(FILE *, File);
  STELA_PRIMITIVE(Opaq);
};

template <>
struct stela::reflect<const char *> {
  STELA_REFLECT_ANON(const char *);
  STELA_PRIMITIVE(Opaq);
};

namespace {

AST makeStdio() {
  AST stdio;
  stdio.name = "stdio";
  Reflector refl;
  refl.reflectPlainCFunc<&fopen>("fopen");
  refl.reflectPlainCFunc<&fputs>("fputs");
  refl.reflectPlainCFunc<&fclose>("fclose");
  refl.appendDeclsTo(stdio.global);
  return stdio;
}

TEST(External_Func, Opaque_ptrs) {
  const char *source = R"(
    import stdio;
  
    type Mode byte;
    let Mode_read = make Mode 0;
    let Mode_write = make Mode 1;
    let Mode_append = make Mode 2;
  
    func getModeStr(mode: Mode) {
      switch mode {
        case Mode_read   return "r\0";
        case Mode_write  return "w\0";
        case Mode_append return "a\0";
        default          return "";
      }
    }
  
    func open(filename: [char], mode: Mode) {
      var modeStr = getModeStr(mode);
      push_back(filename, 0c);
      let file = fopen(data(filename), data(modeStr));
      pop_back(filename);
      return file;
    }
  
    func (stream: File) puts(str: [char]) {
      push_back(str, 0c);
      let ret = fputs(data(str), stream);
      pop_back(str);
    }
  
    func (stream: File) close() {
      let ret = fclose(stream);
    }
  
    extern func sayHi() {
      let file = open("file.txt", Mode_write);
      file.puts("Hello World\n");
      file.close();
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  
  asts.push_back(createAST(source, log()));
  asts.push_back(makeStdio());
  
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  llvm::ExecutionEngine *engine = generate(syms, log());
  
  auto sayHi = GET_FUNC("sayHi", Void());
  const char msg[12] = {'H','e','l','l','o',' ','W','o','r','l','d','\n'};
  
  sayHi();
  std::FILE *file = std::fopen("file.txt", "r");
  ASSERT_TRUE(file);
  std::fseek(file, 0, SEEK_END);
  const long size = std::ftell(file);
  std::rewind(file);
  ASSERT_EQ(size, sizeof(msg));
  
  char realMsg[sizeof(msg)];
  std::fread(realMsg, 1, sizeof(realMsg), file);
  EXPECT_EQ(std::memcmp(msg, realMsg, sizeof(msg)), 0);
  
  std::fclose(file);
  remove("file.txt");
}

TEST(External_func, Conveinience_bindings) {
  const char *source = R"(
    import cmath;
  
    type Vec2 struct {
      x: real;
      y: real;
    };
  
    extern func angleMag(angle: real, mag: real) {
      return make Vec2 {cos(angle) * mag, sin(angle) * mag};
    }
  
    extern func twoZero() {
      return angleMag(0.0, 2.0);
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(source, log()));
  asts.push_back(makeCmath(syms.builtins, log()));
  
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  llvm::ExecutionEngine *engine = generate(syms, log());
  
  struct Vec2 {
    Real x, y;
  };
  
  auto twoZero = GET_FUNC("twoZero", Vec2());
  const Vec2 vec = twoZero();
  EXPECT_EQ(vec.x, 2.0);
  EXPECT_EQ(vec.y, 0.0);
}

TEST(External_func, Simple_bind) {
  const char *source = R"(
    import library;
  
    extern func test(val: real) {
      return calc(val);
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(source, log()));
  
  AST lib;
  lib.name = "library";
  Reflector refl;
  refl.reflectPlainFunc("calc", +[](Real r) {
    return r * 2.0f;
  });
  refl.appendDeclsTo(lib.global);
  asts.push_back(lib);
  
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  llvm::ExecutionEngine *engine = generate(syms, log());
  
  auto test = GET_FUNC("test", Real(Real));
  EXPECT_EQ(test(0.0f), 0.0f);
  EXPECT_EQ(test(-11.0f), -22.0f);
  EXPECT_EQ(test(7.0f), 14.0f);
};

enum class Dir : unsigned char {
  up,
  right,
  down,
  left,
  none = 255
};

} // namespace

template <>
struct stela::reflect<Dir> {
  STELA_REFLECT(Dir);
  STELA_ENUM_TYPE();
  STELA_DECLS(
    STELA_ENUM_CASE(Dir_up,    up),
    STELA_ENUM_CASE(Dir_right, right),
    STELA_ENUM_CASE(Dir_down,  down),
    STELA_ENUM_CASE(Dir_left,  left),
    STELA_ENUM_CASE(Dir_none,  none)
  );
};

namespace {

TEST(External_func, Enums) {
  const char *source = R"(
    import library;
  
    extern func opposite(dir: Dir) {
      switch (dir) {
        case Dir_up return Dir_down;
        case Dir_right return Dir_left;
        case Dir_down return Dir_up;
        case Dir_left return Dir_right;
        default return Dir_none;
      }
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(source, log()));
  
  AST lib;
  lib.name = "library";
  Reflector refl;
  refl.reflectType<Dir>();
  refl.appendDeclsTo(lib.global);
  asts.push_back(lib);
  
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  llvm::ExecutionEngine *engine = generate(syms, log());
  
  auto opposite = GET_FUNC("opposite", Dir(Dir));
  
  EXPECT_EQ(opposite(Dir::up),    Dir::down);
  EXPECT_EQ(opposite(Dir::right), Dir::left);
  EXPECT_EQ(opposite(Dir::down),  Dir::up);
  EXPECT_EQ(opposite(Dir::left),  Dir::right);
  EXPECT_EQ(opposite(Dir::none),  Dir::none);
}

struct Vec2 {
  Real x, y;
  
  Real mag() const {
    return std::sqrt(x*x + y*y);
  }
  void add(const Vec2 other) {
    x += other.x;
    y += other.y;
  }
  
  STELA_REFLECT(Vec2);
  STELA_CLASS(
    STELA_FIELD(x),
    STELA_FIELD(y)
  );
  STELA_DECLS(
    STELA_METHOD(mag),
    STELA_METHOD(add)
  );
};

//struct Vec2 {
//  Real x, y;
//
//  Real mag() const {
//    return std::sqrt(x*x + y*y);
//  }
//  void add(const Vec2 other) {
//    x += other.x;
//    y += other.y;
//  }
//
//  static constexpr std::string_view reflected_name = "Vec2";
//  static inline const auto reflected_type = stela::Class{
//    class_tag_t<Vec2>{},
//    stela::Field<&Vec2::x>{"x"},
//    stela::Field<&Vec2::y>{"y"}
//  };
//  static inline const auto reflected_decl = stela::Decls{
//    stela::Method<&Vec2::mag>{"mag"},
//    stela::Method<&Vec2::add>{"add"}
//  };
//};

TEST(External_func, User_type) {
  const char *source = R"(
    import glm;
  
    extern func test(v: Vec2) {
      var three_four: Vec2;
      three_four.x = 3.0;
      three_four.y = 4.0;
      v.add(three_four);
      return v.mag();
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(source, log()));
  
  AST lib;
  lib.name = "glm";
  Reflector refl;
  refl.reflectType<Vec2>();
  refl.appendDeclsTo(lib.global);
  asts.push_back(lib);
  
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  llvm::ExecutionEngine *engine = generate(syms, log());
  
  auto test = GET_FUNC("test", Real(Vec2));
  
  EXPECT_EQ(test(Vec2{0.0f, 0.0f}), 5.0f);
  EXPECT_EQ(test(Vec2{2.0f, 8.0f}), 13.0f);
  EXPECT_EQ(test(Vec2{4.0f, 20.0f}), 25.0f);
}

} // namespace

template <>
struct reflect<std::shared_ptr<int>> {
  STELA_REFLECT_NAME(std::shared_ptr<int>, SharedInt);
  STELA_CLASS();
  STELA_DECLS(
    STELA_METHOD(get)
  );
};

template <>
struct reflect<int *> {
  STELA_REFLECT_NAME(int *, IntPtr);
  STELA_PRIMITIVE(Opaq);
};

namespace {

TEST(External_func, Shared_ptr) {
  const char *source = R"(
    import memory;
  
    extern func identity(ptr: SharedInt) {
      return ptr;
    }
  
    extern func getOr(ptr: SharedInt, value: sint) {
      var copy = identity(ptr);
      if (copy) {
        return load_ptr(copy.get());
      } else {
        return value;
      }
    }
  
    extern func getNull() {
      var null: SharedInt;
      return null;
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(source, log()));
  
  AST lib;
  lib.name = "memory";
  Reflector refl;
  refl.reflectType<std::shared_ptr<int>>();
  refl.reflectPlainFunc("load_ptr", +[](int *ptr) noexcept {
    assert(ptr);
    return *ptr;
  });
  refl.appendDeclsTo(lib.global);
  asts.push_back(lib);
  
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  llvm::ExecutionEngine *engine = generate(syms, log());
  
  auto identity = GET_FUNC("identity", std::shared_ptr<int>(std::shared_ptr<int>));
  
  auto ptr = std::make_shared<int>(5);
  EXPECT_EQ(ptr.use_count(), 1);
  auto copy = identity(ptr);
  EXPECT_EQ(ptr.use_count(), 2);
  EXPECT_EQ(ptr, copy);
  EXPECT_EQ(*ptr, 5);
  
  auto getOr = GET_FUNC("getOr", int(std::shared_ptr<int>, int));
  
  auto ten = std::make_shared<int>(10);
  EXPECT_EQ(ten.use_count(), 1);
  int ret = getOr(ten, 45);
  EXPECT_EQ(ret, 10);
  EXPECT_EQ(ten.use_count(), 1);
  
  std::shared_ptr<int> null;
  EXPECT_EQ(null.use_count(), 0);
  ret = getOr(null, 7);
  EXPECT_EQ(ret, 7);
  EXPECT_EQ(null.use_count(), 0);
  
  auto getNull = GET_FUNC("getNull", std::shared_ptr<int>());
  
  std::shared_ptr<int> nullPtr = getNull();
  EXPECT_EQ(nullPtr.use_count(), 0);
  EXPECT_FALSE(nullPtr);
}

#undef EXPECT_FAILS
#undef EXPECT_SUCCEEDS
#undef GET_MEM_FUNC
#undef GET_FUNC

}
