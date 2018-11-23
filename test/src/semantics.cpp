//
//  semantics.cpp
//  Test
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "semantics.hpp"

#include "macros.hpp"
#include <STELA/modules.hpp>
#include <STELA/syntax analysis.hpp>
#include <STELA/semantic analysis.hpp>

using namespace stela;
using namespace stela::ast;

Symbols createSym(const std::string_view source, LogBuf &log) {
  AST ast = createAST(source, log);
  Symbols syms = initModules(log);
  compileModule(syms, ast, log);
  return syms;
}

AST makeModuleAST(const ast::Name name, std::vector<ast::Name> &&imports) {
  AST ast;
  ast.name = name;
  ast.imports = std::move(imports);
  return ast;
}

#define ASSERT_SUCCEEDS() createSym(source, log)
#define ASSERT_FAILS() ASSERT_THROWS(createSym(source, log), stela::FatalError)

TEST_GROUP(Semantics, {
  StreamLog log;
  log.pri(LogPri::status);
  
  TEST(Empty source, {
    const Symbols syms = createSym("", log);
    ASSERT_EQ(syms.modules.size(), 2);
    
    auto btn = syms.modules.find("$builtin");
    ASSERT_NE(btn, syms.modules.end());
    const sym::Module &builtin = btn->second;
    ASSERT_EQ(builtin.scopes.size(), 1);
    ASSERT_EQ(builtin.decls.size(), 0);
    
    auto mayn = syms.modules.find("main");
    ASSERT_NE(mayn, syms.modules.end());
    const sym::Module &main = mayn->second;
    ASSERT_EQ(main.scopes.size(), 1);
    ASSERT_EQ(main.decls.size(), 0);
  });
  
  TEST(Modules - Regular, {
    ASTs asts;
    asts.push_back(makeModuleAST("a", {"b", "c"}));
    asts.push_back(makeModuleAST("b", {"d", "e"}));
    asts.push_back(makeModuleAST("c", {}));
    asts.push_back(makeModuleAST("d", {"c"}));
    asts.push_back(makeModuleAST("e", {"d"}));
    asts.push_back(makeModuleAST("f", {"d", "e"}));
    const ModuleOrder order = findModuleOrder(asts, log);
    ASSERT_EQ(order.size(), 6);
    ASSERT_EQ(order[0], 2); // c
    ASSERT_EQ(order[1], 3); // d
    ASSERT_EQ(order[2], 4); // e
    ASSERT_EQ(order[3], 1); // b
    ASSERT_EQ(order[4], 0); // a
    ASSERT_EQ(order[5], 5); // f
  });
  
  TEST(Modules - No imports, {
    ASTs asts;
    asts.push_back(makeModuleAST("a", {}));
    asts.push_back(makeModuleAST("b", {}));
    asts.push_back(makeModuleAST("c", {}));
    asts.push_back(makeModuleAST("d", {}));
    asts.push_back(makeModuleAST("e", {}));
    asts.push_back(makeModuleAST("f", {}));
    const ModuleOrder order = findModuleOrder(asts, log);
    ASSERT_EQ(order.size(), 6);
    ASSERT_EQ(order[0], 0); // a
    ASSERT_EQ(order[1], 1); // b
    ASSERT_EQ(order[2], 2); // c
    ASSERT_EQ(order[3], 3); // d
    ASSERT_EQ(order[4], 4); // e
    ASSERT_EQ(order[5], 5); // f
  });
  
  TEST(Modules - Invalid import, {
    ASTs asts;
    asts.push_back(makeModuleAST("a", {}));
    asts.push_back(makeModuleAST("b", {"aye"}));
    ASSERT_THROWS(findModuleOrder(asts, log), FatalError);
  });
  
  TEST(Modules - Cycle, {
    ASTs asts;
    asts.push_back(makeModuleAST("a", {"b"}));
    asts.push_back(makeModuleAST("b", {"e"}));
    asts.push_back(makeModuleAST("c", {"d"}));
    asts.push_back(makeModuleAST("d", {"e"}));
    asts.push_back(makeModuleAST("e", {"c"}));
    ASSERT_THROWS(findModuleOrder(asts, log), FatalError);
  });
  
  TEST(Modules - Self import, {
    ASTs asts;
    asts.push_back(makeModuleAST("a", {"a"}));
    ASSERT_THROWS(findModuleOrder(asts, log), FatalError);
  });
  
  TEST(Modules - None, {
    ASSERT_TRUE(findModuleOrder({}, log).empty());
  });
  
  TEST(Modules - Import external, {
    ASTs asts;
    asts.push_back(makeModuleAST("a", {"external_a", "b"}));
    asts.push_back(makeModuleAST("b", {"external_b"}));
    ast::Names external;
    external.push_back("external_a");
    external.push_back("external_b");
    const ModuleOrder order = findModuleOrder(asts, external, log);
    ASSERT_EQ(order.size(), 2);
    ASSERT_EQ(order[0], 1); // b
    ASSERT_EQ(order[1], 0); // a
  });
  
  TEST(Modules - Duplicate, {
    ASTs asts;
    asts.push_back(makeModuleAST("a", {}));
    ast::Names external;
    external.push_back("a");
    ASSERT_THROWS(findModuleOrder(asts, external, log), FatalError);
  });
  
  TEST(Modules - Already compiled, {
    const char *source = R"(
      module a;
    )";
    Symbols syms = initModules(log);
    {
      AST ast = createAST(source, log);
      compileModule(syms, ast, log);
    }
    {
      AST ast = createAST(source, log);
      ASSERT_THROWS(compileModule(syms, ast, log), FatalError);
    }
  });
  
  TEST(Modules - Import type, {
    const char *sourceA = R"(
      module ModA;
    
      type Number = real;
      type SpecialNumber = Number;
    )";
    
    const char *sourceB = R"(
      module ModB;
    
      import ModA;
    
      type CoolNumber = ModA::SpecialNumber;
      type Number = CoolNumber;
    
      func main() {
        let n: Number = 5.0;
        let r: real = n;
      }
    )";
    
    Symbols syms = initModules(log);
    ASTs asts;
    asts.push_back(createAST(sourceB, log));
    asts.push_back(createAST(sourceA, log));
    const ModuleOrder order = findModuleOrder(asts, log);
    compileModules(syms, order, asts, log);
  });
  
  TEST(Modules - Import constant, {
    const char *sourceA = R"(
      module ModA;
    
      type Number = real;
    
      let five: Number = 5.0;
    )";
    
    const char *sourceB = R"(
      module ModB;
    
      import ModA;
    
      func main() {
        let n: real = ModA::five;
      }
    )";
    
    Symbols syms = initModules(log);
    ASTs asts;
    asts.push_back(createAST(sourceB, log));
    asts.push_back(createAST(sourceA, log));
    const ModuleOrder order = findModuleOrder(asts, log);
    compileModules(syms, order, asts, log);
  });
  
  TEST(Modules - Import function, {
    const char *sourceA = R"(
      module ModA;
    
      type Number = real;
    
      func getFive() -> Number {
        return 5.0;
      }
    )";
    
    const char *sourceB = R"(
      module ModB;
    
      import ModA;
    
      func main() {
        let n: real = ModA::getFive();
      }
    )";
    
    Symbols syms = initModules(log);
    ASTs asts;
    asts.push_back(createAST(sourceB, log));
    asts.push_back(createAST(sourceA, log));
    const ModuleOrder order = findModuleOrder(asts, log);
    compileModules(syms, order, asts, log);
  });
  
  TEST(Modules - Import struct, {
    const char *sourceA = R"(
      module ModA;
    
      type Number = real;
    
      type Vec2 struct {
        x: Number;
        y: Number;
      };
    
      func getVec2() -> Vec2 {
        var vec: Vec2;
        vec.x = 5.0;
        vec.y = 7.0;
        return vec;
      }
    )";
    
    const char *sourceB = R"(
      module ModB;
    
      import ModA;
    
      func main() {
        let vec: ModA::Vec2 = ModA::getVec2();
        let x: ModA::Number = vec.x;
        let y: real = vec.y;
      }
    )";
    
    Symbols syms = initModules(log);
    ASTs asts;
    asts.push_back(createAST(sourceB, log));
    asts.push_back(createAST(sourceA, log));
    const ModuleOrder order = findModuleOrder(asts, log);
    compileModules(syms, order, asts, log);
  });
  
  TEST(Modules - Forgot to import, {
    const char *sourceA = R"(
      module ModA;
    
      type Number = ModB::Number;
    )";
    
    const char *sourceB = R"(
      module ModB;
    
      import ModA;
    
      type Number = real;
    )";
    
    Symbols syms = initModules(log);
    ASTs asts;
    asts.push_back(createAST(sourceB, log));
    asts.push_back(createAST(sourceA, log));
    const ModuleOrder order = findModuleOrder(asts, log);
    ASSERT_THROWS(compileModules(syms, order, asts, log), FatalError);
  });
  
  log.pri(LogPri::warning);
  
  TEST(Func - Redef, {
    const char *source = R"(
      func myFunction() {
        
      }
      func myFunction() {
        
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Func - Redef with params, {
    const char *source = R"(
      func myFunction(i: sint, f: real) {
        
      }
      func myFunction(i: sint, f: real) {
        
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Func - Redef weak alias, {
    const char *source = R"(
      type Number = sint;
      func myFunction(i: Number) {
        
      }
      func myFunction(i: sint) {
        
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Func - Redef multi weak alias, {
    const char *source = R"(
      type Integer = sint;
      type Number = Integer;
      type SpecialNumber = Number;
      type TheNumber = SpecialNumber;
      func myFunction(i: TheNumber) {
        
      }
      func myFunction(i: sint) {
        
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Func - Redef strong alias, {
    const char *source = R"(
      type Integer = sint;
      type Number Integer;
      type SpecialNumber = Number;
      type TheNumber = SpecialNumber;
      func myFunction(i: TheNumber) {
        
      }
      func myFunction(i: Number) {
        
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Func - Overload strong alias, {
    const char *source = R"(
      type Number real;
      type Float real;
      func myFunction(i: Number) {
        
      }
      func myFunction(i: Float) {
        
      }
    )";
    ASSERT_SUCCEEDS();
  });

  TEST(Func - Overloading, {
    const char *source = R"(
      func myFunction(n: real) {
        
      }
      func myFunction(n: sint) {
        
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Func - Tag dispatch, {
    const char *source = R"(
      type first_t struct{};
      type second_t struct{};
      type third_t struct{};

      let first: first_t = {};
      let second: second_t = {};
      let third: third_t = {};

      func get(t: first_t, arr: [sint]) -> sint {
        return arr[0];
      }
      func get(t: second_t, arr: [sint]) -> sint {
        return arr[1];
      }
      func get(t: third_t, arr: [sint]) -> sint {
        return arr[2];
      }

      func test() {
        let arr = [5, 2, 6];
        let two = get(second, arr);
        let five = get(first, arr);
        let six = get(third, arr);
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Func - Discarded return, {
    const char *source = R"(
      func myFunction() -> sint {
        return 5;
      }
    
      func main() {
        myFunction();
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Func - Recursive, {
    const char *source = R"(
      func fac(n: uint) -> uint {
        return n == 0u ? 1u : n * fac(n - 1u);
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Sym - Undefined, {
    const char *source = R"(
      func myFunction(i: Number) {
        
      }
      func myFunction(i: sint) {
        
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Sym - Colliding type and func, {
    const char *source = R"(
      type fn func();
      func fn() {}
    )";
    ASSERT_FAILS();
  });
  
  TEST(Sym - Type in func, {
    const char *source = R"(
      func fn() {
        type Number = real;
        let n: Number = 4.3;
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Var - Redefine var, {
    const char *source = R"(
      var x: sint;
      var x: real;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Var - Redefine let, {
    const char *source = R"(
      let x: sint = 0;
      let x: real = 0.0;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Var - Type mismatch, {
    const char *source = R"(
      let x: sint = 2.5;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Var - Var equals itself, {
    const char *source = R"(
      let x = x;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Struct - Recursive, {
    const char *source = R"(
      type Struct struct {
        x: Struct;
      };
    )";
    ASSERT_FAILS();
  });
  
  TEST(Struct - Enum, {
    const char *source = R"(
      type Dir sint;
      let Dir_up    = make Dir 0;
      let Dir_right = make Dir 1;
      let Dir_down  = make Dir 2;
      let Dir_left  = make Dir 3;
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Struct - Main, {
    const char *source = R"(
      type Vec struct {
        x: real;
        y: real;
      };
    
      func newVec(x: real, y: real) -> Vec {
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
    
      func (self: inout Vec) div(val: real) {
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
        let two: real = newVec(2.0, 4.0).x;
        let four: real = two + middle.y;
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Struct - More, {
    const char *source = R"(
      type MyString struct {
        s: [char];
      };
    
      func change(string: inout [char]) {
        string = "still a string";
      }
    
      func change(mstr: inout MyString) {
        change(mstr.s);
      }
    
      func main() {
        var thing: MyString;
        thing.s = "a string";
        change(thing);
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Struct - Dup member function, {
    const char *source = R"(
      type MyStruct struct {};
    
      func (self: MyStruct) fn() {}
      func (self: MyStruct) fn() {}
    )";
    ASSERT_FAILS();
  });
  
  TEST(Struct - Member function sint, {
    const char *source = R"(
      func (self: inout sint) zero() {
        self = 0;
      }
    
      func test() {
        var n = 5;
        n.zero();
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Struct - Colliding func field, {
    const char *source = R"(
      type MyStruct struct {
        fn: sint;
      };
    
      func (self: MyStruct) fn() {}
    )";
    ASSERT_FAILS();
  });
  
  TEST(Struct - Dup member, {
    const char *source = R"(
      type MyStruct struct {
        m: sint;
        m: real;
      };
    )";
    ASSERT_FAILS();
  });
  
  TEST(Struct - Access undef field, {
    const char *source = R"(
      type MyStruct struct {
        ajax: sint;
      };
    
      func oh_no() {
        var s: MyStruct;
        s.francis = 4;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Swap literals, {
    const char *source = R"(
      func swap(a: inout sint, b: inout sint) {
        let temp: sint = a;
        a = b;
        b = temp;
      }
    
      func main() {
        swap(4, 5);
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Assign to literal, {
    const char *source = R"(
      func main() {
        '4' = '5';
      }
    )";
    ASSERT_FAILS();
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
    ASSERT_SUCCEEDS();
  });
  
  TEST(Non-bool ternary condition, {
    const char *source = R"(
      func main() {
        let test = ("nope" ? 4 : 6);
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Ternary condition type mismatch, {
    const char *source = R"(
      func main() {
        let test = (true ? 4 : "nope");
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Assign regular type, {
    const char *source = R"(
      type Type struct {};
    
      func main() {
        let t = Type;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Returning an object, {
    const char *source = R"(
      type Deep struct {
        x: sint;
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
        let x: sint = outer.getInner().deep.x;
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Type in function, {
    const char *source = R"(
      func fn() -> sint {
        type MyStruct struct {
          x: sint;
        };
        let nine = make MyStruct {9};
        return nine.x;
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Access type in function, {
    const char *source = R"(
      func fn() {
        type Thing sint;
      }
    
      let thing = make Thing 1;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Nested function, {
    const char *source = R"(
      type Struct struct {};
    
      func fn() -> Struct {
        func nested() -> Struct {
          return make Struct {};
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
    ASSERT_SUCCEEDS();
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
    ASSERT_FAILS();
  });
  
  TEST(Expected type, {
    const char *source = R"(
      let not_a_type = 4;
      var oops: not_a_type;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Must call mem func, {
    const char *source = R"(
      type Struct struct {};
    
      func (self: Struct) fn() {}
    
      var s: Struct;
      let test = s.fn;
    )";
    ASSERT_FAILS();
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
    ASSERT_SUCCEEDS();
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
    ASSERT_SUCCEEDS();
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
    ASSERT_FAILS();
  });
  
  TEST(If - Scope, {
    const char *source = R"(
      func main() {
        if (true) var i = 3;
        i = 2;
      }
    )";
    ASSERT_FAILS();
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
    ASSERT_SUCCEEDS();
  });
  
  TEST(Loops - For var scope, {
    const char *source = R"(
      func main() {
        for (i := 0; i != 10; i++) {}
        i = 3;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Loops - Break outside loop, {
    const char *source = R"(
      func main() {
        break;
      }
    )";
    ASSERT_FAILS();
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
    ASSERT_SUCCEEDS();
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
    ASSERT_FAILS();
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
    ASSERT_FAILS();
  });
  
  TEST(Expr - CompAssign to const, {
    const char *source = R"(
      func main() {
        let constant = 0;
        constant += 4;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - CompAssign bitwise float, {
    const char *source = R"(
      func main() {
        let float = 2.0;
        float |= 4.0;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Increment struct, {
    const char *source = R"(
      func main() {
        var strut: struct{};
        strut++;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Assign diff types, {
    const char *source = R"(
      func main() {
        var num = 5;
        var str = "5";
        num = str;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Unary operators, {
    const char *source = R"(
      func main() {
        let test0 = -(5);
        let test1 = -(5.0);
        let test2 = -('5');
        let test3 = ~(5u);
        let test4 = !(false);
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Bitwise ops, {
    const char *source = R"(
      func main() {
        let five = 5u;
        let ten = five << 1u;
        let two = ten & 2u;
        let t = true;
        let false0 = ten < two;
        let false1 = t && false0;
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Add float int, {
    const char *source = R"(
      func main() {
        let f = 3.0;
        let i = 3;
        let sum = f + i;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Bitwise not float, {
    const char *source = R"(
      func main() {
        let test = ~(3.0);
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Storing void, {
    const char *source = R"(
      func nothing() {}
    
      func main() {
        let void = nothing();
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Access field on void, {
    const char *source = R"(
      func nothing() {}
    
      func main() {
        let value = nothing().field;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Access field on int, {
    const char *source = R"(
      func integer() -> sint {
        return 0;
      }
    
      func main() {
        let value = integer().field;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Call a float, {
    const char *source = R"(
      func main() {
        let fl = 3.0;
        fl(5.0);
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - String literals, {
    const char *source = R"(
      let str: [char] = "This is a string";
      let empty: [char] = "";
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Char literals, {
    const char *source = R"(
      let c: char = 'a';
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Number literals, {
    const char *source = R"(
      let n02: sint = 2147483647;
      let n03: sint = -2147483648;
      let n04: real = -3000000000;
      let n05: uint = 4294967295;
      let n06: real = 5000000000;
      let n07: real = 3.14;
      let n08: real = 3e2;
      let n09: sint = 300;
      let n10: byte = 255b;
      let n11: char = 127c;
      let n12: char = -128c;
      let n13: real = 27r;
      let n14: sint = 3e2s;
      let n15: uint = 4e4u;
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Bool literals, {
    const char *source = R"(
      let t: bool = true;
      let f: bool = false;
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Array literals, {
    const char *source = R"(
      let empty: [sint] = [];
      let nested_empty: [[[real]]] = [[[]]];
      let nested_real: real = nested_empty[0][0][0];
    
      let one: [sint] = [1];
      let two: [sint] = [1, 2];
      let three: [sint] = [1, 2, 3];
      let four: [sint] = [1 + 7, 2 * 3, 11 + (18 / 3)];
    
      type Number = sint;
      let number: Number = 10;
      func getNum() -> Number {
        return 11;
      }
    
      let five: [sint] = [4, number, getNum()];
      let six: [sint] = [number, getNum(), 4];
      let seven: [sint] = [number, getNum()];
    
      let eight: [Number] = [4, number, getNum()];
      let nine: [Number] = [number, getNum(), 4];
      let ten: [Number] = [number, getNum()];
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Array literal only init array, {
    const char *source = R"(
      let number: sint = [1, 2, 3];
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - No infer empty array, {
    const char *source = R"(
      let empty = [];
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Diff types in array, {
    const char *source = R"(
      let arr = [1, 1.0, true];
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Subscript, {
    const char *source = R"(
      type real_array = [real];
      let arr: real_array = [4.2, 11.9, 1.5];
      let second: real = arr[1];
      let third = arr[2];
    
      type index uint;
      let first: real = arr[make index 0];
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Invalid subscript index, {
    const char *source = R"(
      let arr: [bool] = [false, true];
      let nope: bool = arr[1.0];
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Subscript not array, {
    const char *source = R"(
      var not_array: struct{};
      let nope = not_array[0];
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Make init list, {
    const char *source = R"(
      type Vec2 struct {
        x: sint;
        y: sint;
      };
    
      let zero_zero = make Vec2 {};
      let zero_zero_again = make Vec2 {{}, {}};
      let one_two = make Vec2 {1, 2};
      let three_four: Vec2 = {3, 4};
    
      let zero: sint = {};
      let empty: [real] = {};
      let also_empty = make [real] {};
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Invalid make expression, {
    const char *source = R"(
      let thing = make struct{} 5;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Cast between strong structs, {
    const char *source = R"(
      type ToughStruct struct {
        value: sint;
      };
      type StrongBoi struct {
        value: sint;
      };
      let strong = make StrongBoi make ToughStruct {};
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Cast between strong structs diff names, {
    const char *source = R"(
      type ToughStruct struct {
        val: sint;
      };
      type StrongBoi struct {
        value: sint;
      };
      let strong = make StrongBoi make ToughStruct {};
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - No infer init list, {
    const char *source = R"(
      let anything = {};
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Init list can only init structs, {
    const char *source = R"(
      let number: sint = {5};
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Too many expr in init list, {
    const char *source = R"(
      type One struct {
        val: sint;
      };
      let two: One = {1, 2};
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Too few expr in init list, {
    const char *source = R"(
      type Two struct {
        a: sint;
        b: sint;
      };
      let one: Two = {1};
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Init list type mismatch, {
    const char *source = R"(
      type Number real;
      type Struct struct {
        a: Number;
      };
      let s: Struct = {5.0};
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Member function on void, {
    const char *source = R"(
      func getVoid() {}
    
      func memFun(a: sint) -> real {
        return 5.0;
      }
    
      func test() {
        let five: real = getVoid().memFun(5);
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Return real as sint, {
    const char *source = R"(
      func getReal() -> real {
        return 91;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Return strong as concrete, {
    const char *source = R"(
      type StrongEmpty struct{};
    
      func getStrong() -> StrongEmpty {
        return make struct{} {};
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Return void expression, {
    const char *source = R"(
      func getVoid() {}
      func getVoidAgain() {
        return getVoid();
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Address of single function, {
    const char *source = R"(
      type Vec2 struct {
        x: real;
        y: real;
      };
      type Vector = Vec2;
    
      func one(a: real, b: inout real, c: Vector) -> Vec2 {
        b = a;
        return make Vector {c.x * a, c.y + a};
      }
    
      let ptr0 = one;
      let ptr1: func(real, inout real, Vector) -> Vec2 = one;
      let ptr2: func(real, inout real, Vec2) -> Vector = one;
      let ptr3 = make func(real, inout real, Vector) -> Vec2 one;
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Address of overloaded function, {
    const char *source = R"(
      func add(a: sint, b: sint) -> sint {
        return a + b;
      }
      func add(a: uint, b: uint) -> uint {
        return a + b;
      }
      func add(a: real, b: real) -> real {
        return a + b;
      }
    
      let ptr0: func(real, real) -> real = add;
      let ptr1 = make func(sint, sint) -> sint add;
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Address of single member function, {
    const char *source = R"(
      func (self: sint) memfun(param: real) -> real {
        return make real self * param;
      }
    
      let ptr = memfun;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Address of overloaded member function, {
    const char *source = R"(
      func (self: sint) memfun(param: real) -> real {
        return make real self * param;
      }
      func (self: sint) memfun(param: uint) -> uint {
        return make uint self * param;
      }
      func (self: real) memfun(param: sint) -> real {
        return self * make real param;
      }
    
      let ptr: func(uint) -> uint = memfun;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Ambiguous overloaded function, {
    const char *source = R"(
      func add(a: sint, b: sint) -> sint {
        return a + b;
      }
      func add(a: uint, b: uint) -> uint {
        return a + b;
      }
      func add(a: real, b: real) -> real {
        return a + b;
      }
    
      let ptr = add;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Single function no match signature, {
    const char *source = R"(
      func add(a: sint, b: sint) -> sint {
        return a + b;
      }
    
      let ptr: func(real, sint) -> sint = add;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Overloaded function no match signature, {
    const char *source = R"(
      func add(a: sint, b: sint) -> sint {
        return a + b;
      }
      func add(a: uint, b: uint) -> uint {
        return a + b;
      }
      func add(a: real, b: real) -> real {
        return a + b;
      }
    
      let ptr: func(real, sint) -> sint = add;
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Address function void return, {
    const char *source = R"(
      func getVoid(a: sint) {}
    
      let ptr: func(sint) = getVoid;
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Func - Nested recursive, {
    const char *source = R"(
      func fn(a: sint) {
        fn(3);
        func fn(a: real) {
          fn(3.0);
          func fn(a: char) {
            fn('3');
          }
        }
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Func address nested recursive, {
    const char *source = R"(
      func fn(a: sint) {
        let ptr: func(sint) = fn;
        func fn(a: real) {
          let ptr: func(real) = fn;
          func fn(a: char) {
            let ptr: func(char) = fn;
          }
        }
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Func - Nested recursive no shadow, {
    const char *source = R"(
      func fn_sint(a: sint) {
        fn_sint(3);
        func fn_real(a: real) {
          fn_sint(3);
          fn_real(3.0);
          func fn_char(a: char) {
            fn_sint(3);
            fn_real(3.0);
            fn_char('3');
          }
          fn_sint(3);
          fn_real(3.0);
          fn_char('3');
        }
        fn_sint(3);
        fn_real(3.0);
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Expr - Func address nested recursive no shadow, {
    const char *source = R"(
      func fn_sint(a: sint) {
        let ptr0 = fn_sint;
        func fn_real(a: real) {
          let ptr0 = fn_sint;
          let ptr1 = fn_real;
          func fn_char(a: char) {
            let ptr0 = fn_sint;
            let ptr1 = fn_real;
            let ptr2 = fn_char;
          }
          let ptr2 = fn_sint;
          let ptr3 = fn_real;
          let ptr4 = fn_char;
        }
        let ptr1 = fn_sint;
        let ptr2 = fn_real;
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Func - Cannot call shadowed function, {
    const char *source = R"(
      func fn(a: sint) {
        func fn(a: real) {
          func fn(a: char) {
            fn(3.0);
          }
        }
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Cannot address shadowed function, {
    const char *source = R"(
      func fn(a: sint) {
        func fn(a: real) {
          func fn(a: char) {
            let ptr: func(real) = fn;
          }
        }
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Func - Cannot call shadowed parent, {
    const char *source = R"(
      func fn(a: sint) {
        func fn(a: real) {}
        fn(3);
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - Cannot address shadowed parent, {
    const char *source = R"(
      func fn(a: sint) {
        func fn(a: real) {}
        let ptr: func(sint) = fn;
      }
    )";
    ASSERT_FAILS();
  });

  TEST(Func - Undefined member function, {
    const char *source = R"(
      func test() {
        let x = 10;
        x.undefined();
      }
    )";
    ASSERT_FAILS();
  });

  TEST(Undefined symbol, {
    const char *source = R"(
      func test() {
        let x = 10;
        x = undefined;
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Unreachable variable, {
    const char *source = R"(
      func test(unreachable: sint) {
        func inner() {
          unreachable = 1;
        }
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Calling type, {
    const char *source = R"(
      type MyStruct struct {};
      func test() {
        // this looks like a function call
        // that's why I chose the make syntax
        let instance = MyStruct();
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Access type in nested function, {
    const char *source = R"(
      type GlobalType struct{};
      func outer() {
        type OuterType struct{};
        func inner(a: GlobalType, b: OuterType) {
          let c = make GlobalType {};
          let d = make OuterType {};
        }
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Assign to function paramenter, {
    const char *source = R"(
      func fn(a: sint, b: inout sint) {
        a = 1;
        b = 2;
      }
    )";
    ASSERT_SUCCEEDS();
  });

  TEST(Lambda, {
    const char *source = R"(
      func giveMeFunction(fn: func(real)->sint) {
        let result: sint = fn(2.0);
      }
    
      func test() {
        giveMeFunction(func(a: real) -> sint {
          return 11 * make sint a;
        });
        func notLambda(a: real) -> sint {
          return 11 * make sint a;
        }
        giveMeFunction(notLambda);
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  TEST(Lambda wrong args, {
    const char *source = R"(
      func test() {
        let lam = func(a: sint) -> sint {
          return a * 2;
        };
        let double = lam(16.3);
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Unused lambda return, {
    const char *source = R"(
      func test() {
        let identity = func(a: sint) -> sint {
          return a;
        };
        identity(11);
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Partial application, {
    const char *source = R"(
      func makeAdder(left: sint) -> func(sint) -> sint {
        return func(right: sint) -> sint {
          return left + right;
        };
      }
    
      func test() {
        let add3 = makeAdder(3);
        let nine = add3(6);
        let eight = makeAdder(6)(2);
      }
    )";
    ASSERT_SUCCEEDS();
  });

  /*
  
  Perhaps control flow analysis should be handled by code generation
  
  TEST(Expr - Missing return, {
    const char *source = R"(
      func getSomething() -> sint {}
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - May not return, {
    const char *source = R"(
      func maybe3(b: bool) -> sint {
        if (b) {
          return 3;
        }
      }
    )";
    ASSERT_FAILS();
  });
  
  TEST(Expr - If will return, {
    const char *source = R"(
      func willReturn(b: bool) -> sint {
        if (b) {
          return 3;
        } else {
          return -1;
        }
      }
    )";
    ASSERT_SUCCEEDS();
  });
  
  */

});

#undef ASSERT_SUCCEEDS
#undef ASSERT_FAILS
