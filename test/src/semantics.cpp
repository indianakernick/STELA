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
  log.pri(LogPri::warning);
  
  TEST(Empty source, {
    const auto [symbols, ast] = createSym("", log);
    ASSERT_EQ(symbols.scopes.size(), 2);
    ASSERT_TRUE(ast.global.empty());
    ASSERT_EQ(ast.builtin.size(), 14);
  });
  
  TEST(Func - Redef, {
    const char *source = R"(
      func myFunction() {
        
      }
      func myFunction() {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Func - Redef with params, {
    const char *source = R"(
      func myFunction(i: Int, f: Float) {
        
      }
      func myFunction(i: Int, f: Float) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Func - Redef weak alias, {
    const char *source = R"(
      type Number = Int;
      func myFunction(i: Number) {
        
      }
      func myFunction(i: Int) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Func - Redef multi weak alias, {
    const char *source = R"(
      type Integer = Int;
      type Number = Integer;
      type SpecialNumber = Number;
      type TheNumber = SpecialNumber;
      func myFunction(i: TheNumber) {
        
      }
      func myFunction(i: Int) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Func - Redef strong alias, {
    const char *source = R"(
      type Integer = Int;
      type Number Integer;
      type SpecialNumber = Number;
      type TheNumber = SpecialNumber;
      func myFunction(i: TheNumber) {
        
      }
      func myFunction(i: Number) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Func - Overload strong alias, {
    const char *source = R"(
      type Number Float;
      type Real Float;
      func myFunction(i: Number) {
        
      }
      func myFunction(i: Real) {
        
      }
    )";
    createSym(source, log);
  });

  TEST(Func - Overloading, {
    const char *source = R"(
      func myFunction(n: Float) {
        
      }
      func myFunction(n: Int) {
        
      }
    )";
    createSym(source, log);
  });
  
  TEST(Func - Discarded return, {
    const char *source = R"(
      func myFunction() -> Int {
        return 5;
      }
    
      func main() {
        myFunction();
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Sym - Undefined, {
    const char *source = R"(
      func myFunction(i: Number) {
        
      }
      func myFunction(i: Int) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Sym - Colliding type and func, {
    const char *source = R"(
      type fn func();
      func fn() {}
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Var - Var type inference, {
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
  
  TEST(Var - Let type inference, {
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
  
  TEST(Var - Big num type inference, {
    const char *source = R"(
      let big = 18446744073709551615;
      let neg = -9223372036854775807;
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
  
  TEST(Var - exprs, {
    const char *source = R"(
      let num: Int = 0;
      //var f: (Float, inout String) -> [Double];
      //var m: [{Float: Int}];
      //let a: [Float] = [1.2, 3.4, 5.6];
    )";
    createSym(source, log);
  });
  
  TEST(Var - Redefine var, {
    const char *source = R"(
      var x: Int;
      var x: Double;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Var - Redefine let, {
    const char *source = R"(
      let x: Int = 0;
      let x: Double = 0.0;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Var - Type mismatch, {
    const char *source = R"(
      let x: Int = 2.5;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Var - Var equals itself, {
    const char *source = R"(
      let x = x;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Main, {
    const char *source = R"(
      type Vec struct {
        x: Double;
        y: Double;
      };
    
      func newVec(x: Double, y: Double) -> Vec {
        var self: Vec;
        self.x = x;
        self.y = y;
        return self;
      }
        
      let origin = newVec(0.0, 0.0);
        
      func (self: inout Vec) add(other: Vec) {
        self.x += other.x;
        self.y += other.y;
      }
    
      func (self: inout Vec) div(val: Double) {
        self.x /= val;
        self.y /= val;
      }
    
      func mid(a: Vec, b: Vec) -> Vec {
        var ret = a;
        ret.add(b);
        ret.div(2.0);
        return ret;
      }
    
      func main() {
        let middle: Vec = mid(origin, newVec(2.0, 4.0));
        let two: Double = newVec(2.0, 4.0).x;
        let four: Double = two + middle.y;
      }
    )";
    createSym(source, log);
  });
  
  TEST(Struct - More, {
    const char *source = R"(
      type MyString struct {
        s: String;
      };
    
      func append(string: inout String) {
        string += " is still a string";
      }
    
      // rename this function to appendImpl and it fails
      // raname it to append and it succeeds
      func append(mstr: inout MyString) {
        append(mstr.s);
      }
    
      func main() {
        var thing: MyString;
        thing.s = "A string";
        append(thing);
      }
    )";
    createSym(source, log);
  });
  
  TEST(Struct - Dup member function, {
    const char *source = R"(
      type MyStruct struct {};
    
      func (self: MyStruct) fn() {}
      func (self: MyStruct) fn() {}
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Colliding func field, {
    const char *source = R"(
      type MyStruct struct {
        fn: Int;
      };
    
      func (self: MyStruct) fn() {}
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Dup member, {
    const char *source = R"(
      type MyStruct struct {
        m: Int;
        m: Float;
      };
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Access undef field, {
    const char *source = R"(
      type MyStruct struct {
        ajax: Int;
      };
    
      func oh_no() {
        var s: MyStruct;
        s.francis = 4;
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
  
  TEST(Non-bool ternary condition, {
    const char *source = R"(
      func main() {
        let test = ("nope" ? 4 : 6);
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Ternary condition type mismatch, {
    const char *source = R"(
      func main() {
        let test = (true ? 4 : "nope");
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Assign regular type, {
    const char *source = R"(
      type Type struct {};
    
      func main() {
        let t = Type;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Returning an object, {
    const char *source = R"(
      type Deep struct {
        x: Int;
      };
      type Inner struct {
        deep: Deep;
      };
      type Outer struct {
        inner: Inner;
      };
    
      func (self: Outer) getInner() -> Inner {
        return self.inner;
      }
    
      func main() {
        var outer: Outer;
        let x: Int = outer.getInner().deep.x;
      }
    )";
    createSym(source, log);
  });
  
  TEST(Nested function, {
    const char *source = R"(
      type Struct struct {};
    
      func fn() -> Struct {
        func nested() -> Struct {
          var s: Struct;
          return s;
        }
        return nested();
      }
    
      func main() {
        func nested() -> Struct {
          return fn();
        }
        let s: Struct = nested();
      }
    )";
    createSym(source, log);
  });
  
  TEST(Nested function access, {
    const char *source = R"(
      func main() {
        var nope = 5;
        {
          func nested() {
            nope = 7;
          }
        }
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expected type, {
    const char *source = R"(
      let not_a_type = 4;
      var oops: not_a_type;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Must call free func, {
    const char *source = R"(
      func fn() {}
      let test = fn;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Must call mem func, {
    const char *source = R"(
      type Struct struct {};
    
      func (self: Struct) fn() {}
    
      var s: Struct;
      let test = s.fn;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(If - Standard, {
    const char *source = R"(
      func main() {
        var a = 4;
        var b = 4;
        if (a == b) {
          a += b;
        }
      }
    )";
    createSym(source, log);
  });
  
  TEST(If - Else, {
    const char *source = R"(
      func main() {
        var a = 4;
        var b = 4;
        if (a == b) {
          a += b;
        } else if (a < b) {
          b -= a;
        }
      }
    )";
    createSym(source, log);
  });
  
  TEST(If - Non-bool, {
    const char *source = R"(
      func main() {
        var a = 4;
        if (a) {
          a = 0;
        }
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(If - Scope, {
    const char *source = R"(
      func main() {
        if (true) var i = 3;
        i = 2;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Loops - All, {
    const char *source = R"(
      func main() {
        while (true) {
          if (false) {
            continue;
          } else {
            break;
          }
        }
        for (i := 0; i != 10; i++) {}
      }
    )";
    createSym(source, log);
  });
  
  TEST(Loops - For var scope, {
    const char *source = R"(
      func main() {
        for (i := 0; i != 10; i++) {}
        i = 3;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Loops - Break outside loop, {
    const char *source = R"(
      func main() {
        break;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Switch - Standard, {
    const char *source = R"(
      func main() {
        let num = 3;
        switch (num) {}
        switch (num) {
          case (3) {
            continue;
          }
          case (4) {
            break;
          }
          case (5) {}
        }
        switch (num) {
          case (3) {}
          default {}
        }
      }
    )";
    createSym(source, log);
  });
  
  TEST(Switch - Multiple default, {
    const char *source = R"(
      func main() {
        let num = 3;
        switch (num) {
          default {}
          case (3) {}
          default {}
        }
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Switch - Type mismatch, {
    const char *source = R"(
      func main() {
        let num = 3;
        switch (num) {
          default {}
          case ("oops") {}
        }
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expr - CompAssign to const, {
    const char *source = R"(
      func main() {
        let constant = 0;
        constant += 4;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expr - CompAssign bitwise float, {
    const char *source = R"(
      func main() {
        let float = 2.0;
        float |= 4.0;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expr - Increment struct, {
    const char *source = R"(
      func main() {
        var strut: struct{};
        strut++;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expr - Assign diff types, {
    const char *source = R"(
      func main() {
        var num = 5;
        var str = "5";
        num = str;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expr - Unary operators, {
    const char *source = R"(
      func main() {
        let test0 = -(5);
        let test1 = -(5.0);
        let test2 = -('5');
        let test3 = ~(5);
        let test4 = !(false);
      }
    )";
    createSym(source, log);
  });
  
  TEST(Expr - Bitwise ops, {
    const char *source = R"(
      func main() {
        let five = 5;
        let ten = five << 1;
        let two = ten & 2;
        let t = true;
        let false0 = ten < two;
        let false1 = t && false0;
      }
    )";
    createSym(source, log);
  });
  
  TEST(Expr - Add float int, {
    const char *source = R"(
      func main() {
        let f = 3.0;
        let i = 3;
        let sum = f + i;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expr - Bitwise not float, {
    const char *source = R"(
      func main() {
        let test = ~(3.0);
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
});
