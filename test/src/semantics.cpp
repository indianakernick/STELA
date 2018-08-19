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
  
  TEST(Func - Redef alias, {
    const char *source = R"(
      typealias Number = Int;
      func myFunction(i: Number) {
        
      }
      func myFunction(i: Int) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Func - Redef multi alias, {
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

  TEST(Sym - Undefined, {
    const char *source = R"(
      func myFunction(i: Number) {
        
      }
      func myFunction(i: Int) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
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
  
  TEST(Sym - Colliding type and function, {
    const char *source = R"(
      struct fn {}
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
      var x: Float;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Var - Redefine let, {
    const char *source = R"(
      let x: Int = 0;
      let x: Float = 0.0;
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
      struct Vec {
        var x: Double;
        var y: Double;
        
        init(x: Double, y: Double) {
          self.x = x;
          self.y = y;
        }
        
        static var origin = Vec(0.0, 0.0);
        
        mutating func add(other: Vec) {
          self.x += other.x;
          self.y += other.y;
        }
        mutating func div(val: Double) {
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
        let middle = mid(Vec.origin, Vec(2.0, 3.0));
        let two = (Vec(2.0, 3.0)).x;
        let three = two + 1.0;
      }
    )";
    createSym(source, log);
  });
  
  TEST(Struct - More, {
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
        init() {}
        var s: String;
      }
    
      func append(str: inout String) {
        str += " is still a string";
      }
    
      func append(str: inout MyString) {
        append(str.s);
      }
    
      func main() {
        var thing = MyString();
        thing.s = "A string";
        append(thing);
      }
    )";
    createSym(source, log);
  });
  
  TEST(Struct - Static init, {
    const char *source = R"(
      struct MyStruct {
        static init() {
          // this is not allowed to be static
        }
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Overload static and non-static, {
    const char *source = R"(
      struct MyStruct {
        func fn() {}
        static func fn() {}
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Dup member function, {
    const char *source = R"(
      struct MyStruct {
        func fn() {}
        func fn() {}
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Colliding function and variable, {
    const char *source = R"(
      struct MyStruct {
        var fn: Int;
        func fn() {}
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Dup member, {
    const char *source = R"(
      struct MyStruct {
        var m: Int;
        var m: Float;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Construct nested, {
    const char *source = R"(
      struct Outer {
        struct Inner {
          init() {}
        }
      }
      let inner: Outer.Inner = Outer.Inner();
    )";
    createSym(source, log);
  });
  
  TEST(Struct - Access control, {
    const char *source = R"(
      struct MyStruct {
        init() {}
      
        private var priv: Int;
        
        mutating func increase(amount: Int) {
          self.priv += amount;
        }
        private func get() -> Int {
          return self.priv;
        }
        func getDouble() -> Int {
          return self.get() * 2;
        }
        
        struct Half {
          init() {}
        }
        struct Third {
          init() {}
        }
        
        private func getFrac(h: MyStruct.Half) -> Int {
          return self.priv / 2;
        }
        func getFrac(t: MyStruct.Third) -> Int {
          return self.priv / 3;
        }
        
        static func getFive() -> MyStruct {
          var s = MyStruct();
          s.priv = 5;
          return s;
        }
      }
    
      func main() {
        var five = MyStruct.getFive();
        let third: Int = five.getFrac(MyStruct.Third());
        five.increase(2);
        let dub: Int = five.getDouble();
      }
    )";
    createSym(source, log);
  });
  
  TEST(Struct - Access private from outside, {
    const char *source = R"(
      struct MyStruct {
        private var priv: Int;
      }
    
      func oh_no() {
        var s = MyStruct();
        s.priv = 4;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Access private from nested, {
    const char *source = R"(
      struct MyStruct {
        private func priv() -> Int {
          return 7;
        }
        
        struct Nested {
          static func getPriv(s: MyStruct) -> Int {
            return s.priv();
          }
        }
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Access undefined member var, {
    const char *source = R"(
      struct MyStruct {
        init() {}
        var ajax: Int;
      }
    
      func oh_no() {
        var s = MyStruct();
        s.francis = 4;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Implicitly access static, {
    const char *source = R"(
      struct Struct {
        static func statFun() {}
        static var statVar: Int;
        
        func test() {
          statFun();
          statVar = 4;
        }
      }
    )";
    createSym(source, log);
  });
  
  TEST(Enum - Basic, {
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
  
  TEST(Enum - Access undefined case, {
    const char *source = R"(
      enum Enum {
        ay
      }
    
      func oh_no() {
        let bee = Enum.bee;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Enum - Duplicate case, {
    const char *source = R"(
      enum Dups {
        ay,
        bee,
        see,
        bee
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Enum - Assign to case, {
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
  
  TEST(Enum - Case value, {
    const char *source = R"(
      enum Enum {
        one = 1,
        also_one = one,
        two,
        four = 4,
        five
      }
    )";
    createSym(source, log);
  });
  
  TEST(Enum - Case equals itself, {
    const char *source = R"(
      enum Enum {
        me = me
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Enum - Case equals string, {
    const char *source = R"(
      enum Enum {
        str = "str"
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Enum - Case equals type, {
    const char *source = R"(
      enum Enum {
        type = Int
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Free init, {
    const char *source = R"(
      init() {}
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
        ("nope" ? 4 : 6);
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Ternary condition type mismatch, {
    const char *source = R"(
      func main() {
        (true ? 4 : "nope");
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Nested types, {
    const char *source = R"(
      struct Outer {
        struct Inner {
          static let five = 5;
          
          var num: Int = five;
        }
      }
    
      func main() {
        let alsoFive = Outer.Inner.five;
      }
    )";
    createSym(source, log);
  });
  
  TEST(Assign nested type, {
    const char *source = R"(
      struct Outer {
        struct Inner {
          static let five = 5;
          
          var num: Double;
        }
      }
    
      func main() {
        let inner = Outer.Inner;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Assign regular type, {
    const char *source = R"(
      struct Type {}
    
      func main() {
        let type = Type;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Returning an object, {
    const char *source = R"(
      struct Outer {
        init() {}
        struct Inner {
          init() {}
          struct Deep {
            var x: Int;
          }
          
          var deep: Deep;
        }
        func getInner() -> Inner {
          return Inner();
        }
      }
    
      func main() {
        let outer = Outer();
        let inner: Outer.Inner = outer.getInner();
        let deep: Outer.Inner.Deep = outer.getInner().deep;
        let x: Int = outer.getInner().deep.x;
      }
    )";
    createSym(source, log);
  });
  
  TEST(Nested function, {
    const char *source = R"(
      struct Struct {
        init() {}
        static func fn() -> Struct {
          func nested() -> Struct {
            return Struct();
          }
          return nested();
        }
      }
    
      func main() {
        func nested() -> Struct {
          return Struct.fn();
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
  
  TEST(Must call inst mem func, {
    const char *source = R"(
      struct Struct {
        init() {}
        func fn() {}
      }
    
      let s = Struct();
      let test = s.fn;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Must call inst mem func, {
    const char *source = R"(
      struct Struct {
        static func fn() {}
      }
    
      let test = Struct.fn;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Access static var in instance, {
    const char *source = R"(
      struct Struct {
        init() {}
        static var stat = 4;
      }
    
      let instance = Struct();
      let test = instance.stat;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Access instance var in struct, {
    const char *source = R"(
      struct Struct {
        var inst = 4;
      }
    
      let test = Struct.inst;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Access static func in instance, {
    const char *source = R"(
      struct Struct {
        init() {}
        static func stat() {}
      }
    
      let instance = Struct();
      let test = instance.stat();
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Access instance func in struct, {
    const char *source = R"(
      struct Struct {
        func inst() {}
      }
    
      let test = Struct.inst();
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
  
  TEST(Loops - All, {
    const char *source = R"(
      func main() {
        while (true) {}
        repeat {} while (true);
        for (var i = 0; i != 10; ++i) {}
      }
    )";
    createSym(source, log);
  });
  
  TEST(Loops - For var scope, {
    const char *source = R"(
      func main() {
        {for (var i = 0; i != 10; ++i) {}}
        i = 3;
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
          case (3) {}
          case (4) {}
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
});
