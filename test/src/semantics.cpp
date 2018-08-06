//
//  semantics.cpp
//  Test
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "semantics.hpp"

#include "macros.hpp"
#include <STELA/semantic analysis.hpp>

using namespace stela;
using namespace stela::ast;

TEST_GROUP(Semantics, {
  StreamLog log;
  log.pri(LogPri::verbose);
  
  TEST(Empty source, {
    const auto [symbols, ast] = createSym("", log);
    ASSERT_EQ(symbols.scopes.size(), 2);
    //ASSERT_TRUE(symbols.scopes[1]->table.empty());
    ASSERT_TRUE(ast.global.empty());
  });
  
  /*TEST(Return type inference, {
    const char *source = R"(
      func returnInt() {
        return 5;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });*/
  
  TEST(Redef func, {
    const char *source = R"(
      func myFunction() {
        
      }
      func myFunction() {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Redef func with params, {
    const char *source = R"(
      func myFunction(i: Int, f: Float) {
        
      }
      func myFunction(i: Int, f: Float) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Redef func alias, {
    const char *source = R"(
      typealias Number = Int;
      func myFunction(i: Number) {
        
      }
      func myFunction(i: Int) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Redef func multi alias, {
    const char *source = R"(
      typealias Integer = Int;
      typealias Number = Integer;
      typealias SpecialNumber = Number;
      typealias TheNumber = SpecialNumber;
      func myFunction(i: TheNumber) {
        
      }
      func myFunction(i: Int) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });

  TEST(Undefined symbol, {
    const char *source = R"(
      func myFunction(i: Number) {
        
      }
      func myFunction(i: Int) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });

  TEST(Function overloading, {
    const char *source = R"(
      func myFunction(n: Float) {
        
      }
      func myFunction(n: Int) {
        
      }
    )";
    createSym(source, log);
  });
  
  TEST(Var type inference, {
    const char *source = R"(
      var num = 5;
    )";
    const auto [symbols, ast] = createSym(source, log);
    /*ASSERT_EQ(symbols.scopes.size(), 2);
    const sym::Table &global = symbols.scopes[1]->table;
    ASSERT_EQ(global.size(), 1);
    const auto iter = global.find("num");
    ASSERT_NE(iter, global.end());
    auto *obj = ASSERT_DOWN_CAST(const sym::Object, iter->second.get());
    ASSERT_EQ(obj->etype.cat, sym::ValueCat::lvalue_var);
    auto *builtin = ASSERT_DOWN_CAST(const sym::BuiltinType, obj->etype.type);
    ASSERT_EQ(builtin->value, sym::BuiltinType::Int64);*/
  });
  
  TEST(Let type inference, {
    const char *source = R"(
      let pi = 3.14;
    )";
    const auto [symbols, ast] = createSym(source, log);
    /*ASSERT_EQ(symbols.scopes.size(), 2);
    const sym::Table &global = symbols.scopes[1]->table;
    ASSERT_EQ(global.size(), 1);
    const auto iter = global.find("pi");
    ASSERT_NE(iter, global.end());
    auto *obj = ASSERT_DOWN_CAST(const sym::Object, iter->second.get());
    ASSERT_EQ(obj->etype.cat, sym::ValueCat::lvalue_let);
    auto *builtin = ASSERT_DOWN_CAST(const sym::BuiltinType, obj->etype.type);
    ASSERT_EQ(builtin->value, sym::BuiltinType::Double);*/
  });
  
  TEST(Big num type inference, {
    const char *source = R"(
      let big = 18446744073709551615;
    )";
    const auto [symbols, ast] = createSym(source, log);
    /*ASSERT_EQ(symbols.scopes.size(), 2);
    const sym::Table &global = symbols.scopes[1]->table;
    ASSERT_EQ(global.size(), 1);
    const auto iter = global.find("big");
    ASSERT_NE(iter, global.end());
    auto *obj = ASSERT_DOWN_CAST(const sym::Object, iter->second.get());
    ASSERT_EQ(obj->etype.cat, sym::ValueCat::lvalue_let);
    auto *builtin = ASSERT_DOWN_CAST(const sym::BuiltinType, obj->etype.type);
    ASSERT_EQ(builtin->value, sym::BuiltinType::UInt64);*/
  });
  
  TEST(Variables, {
    const char *source = R"(
      let num: Int = 0;
      //var f: (Float, inout String) -> [Double];
      //var m: [{Float: Int}];
      //let a: [Float] = [1.2, 3.4, 5.6];
    )";
    createSym(source, log);
  });
  
  TEST(Redefine var, {
    const char *source = R"(
      var x: Int;
      var x: Float;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Redefine let, {
    const char *source = R"(
      let x: Int = 0;
      let x: Float = 0.0;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Bad variable decl, {
    const char *source = R"(
      let x: Int = 2.5;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Structs, {
    const char *source = R"(
      struct Vec {
        var x: Double;
        var y: Double;
        
        init(x: Double, y: Double) {
          self.x = x;
          self.y = y;
        }
        
        static var origin = make Vec(0.0, 0.0);
        
        func add(other: Vec) {
          self.x += other.x;
          self.y += other.y;
        }
        func div(val: Double) {
          self.x /= val;
          self.y /= val;
        }
      };
    
      func mid(a: Vec, b: Vec) -> Vec {
        var ret = a;
        ret.add(b);
        ret.div(2.0);
        return ret;
      }
    
      func main() {
        let middle = mid(Vec.origin, make Vec(2.0, 3.0));
        let two = (make Vec(2.0, 3.0)).x;
        let three = two + 1.0;
      }
    )";
    createSym(source, log);
  });
  
  TEST(More Structs, {
    const char *source = R"(
      struct Vec {
        static typealias StaticIsRedundant = Int;
        static enum StaticIsRedundant1 {
          a, b, c
        }
        static struct StaticIsRedundant2 {
          var mem: String;
        }
      }
    
      struct MyString {
        var s: String;
      }
    
      func append(str: inout String) {
        str += " is still a string";
      }
    
      func append(str: inout MyString) {
        append(str.s);
      }
    
      func main() {
        var thing = make MyString();
        thing.s = "A string";
        append(thing);
      }
    )";
    createSym(source, log);
  });
  
  TEST(Static init, {
    const char *source = R"(
      struct MyStruct {
        static init() {
          // this is not allowed to be static
        }
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Enums, {
    const char *source = R"(
      enum Dir {
        up,
        right,
        down,
        left
      };
    
      let south = Dir.down;
      let east: Dir = Dir.right;
    )";
    createSym(source, log);
  });
  
  TEST(Assign to enum, {
    const char *source = R"(
      enum Eeeeenum {
        e
      };
    
      func main() {
        Eeeeenum.e = !true;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Swap literals, {
    const char *source = R"(
      func swap(a: inout Int, b: inout Int) {
        let temp: Int = a;
        a = b;
        b = temp;
      }
    
      func main() {
        swap(4, 5);
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Assign to literal, {
    const char *source = R"(
      func main() {
        '4' = '5';
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Assign to ternary, {
    const char *source = R"(
      func main() {
        var a = 5;
        {
          var b = 7;
          {
            (a < b ? a : b) = (a > b ? a : b);
          }
        }
      }
    )";
    createSym(source, log);
  });
});
