//
//  semantics.cpp
//  Test
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include <gtest/gtest.h>
#include <STELA/modules.hpp>
#include <STELA/syntax analysis.hpp>
#include <STELA/semantic analysis.hpp>
#include <STELA/c standard library.hpp>

using namespace stela;
using namespace stela::ast;

namespace {

Symbols createSym(const std::string_view source, LogSink &log) {
  AST ast = createAST(source, log);
  Symbols syms = initModules(log);
  compileModule(syms, ast, log);
  return syms;
}

AST makeModuleAST(const ast::Name name, ast::Names &&imports) {
  AST ast;
  ast.name = name;
  ast.imports = std::move(imports);
  return ast;
}

LogSink &log() {
  static StreamSink stream;
  static FilterSink filter{stream, LogPri::info};
  return filter;
}

class Modules : public ::testing::Test {
protected:
  Modules()
    : stream{}, filter{stream, LogPri::status} {}
  
  LogSink &log() {
    return filter;
  }

private:
  StreamSink stream;
  FilterSink filter;
};

#define EXPECT_SUCCEEDS(SOURCE) createSym(SOURCE, log())
#define EXPECT_FAILS(SOURCE) EXPECT_THROW(createSym(SOURCE, log()), stela::FatalError)

TEST(Basic, Empty_source) {
  const Symbols syms = createSym("", log());
  EXPECT_TRUE(syms.decls.empty());
  EXPECT_EQ(syms.scopes.size(), 2);
  EXPECT_EQ(syms.global, syms.scopes[1].get());
  EXPECT_EQ(syms.global->parent, syms.scopes[0].get());
  EXPECT_FALSE(syms.global->parent->parent);
  
  EXPECT_FALSE(syms.scopes[0]->table.empty());
  EXPECT_TRUE(syms.scopes[1]->table.empty());
}

TEST_F(Modules, Scope_graph) {
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(makeModuleAST("a", {}));
  asts.push_back(makeModuleAST("b", {"a"}));
  ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
  
  EXPECT_TRUE(syms.decls.empty());
  EXPECT_EQ(syms.scopes.size(), 3);
  EXPECT_EQ(syms.global, syms.scopes[2].get());
  EXPECT_EQ(syms.global->parent, syms.scopes[1].get());
  EXPECT_EQ(syms.global->parent->parent, syms.scopes[0].get());
  EXPECT_FALSE(syms.global->parent->parent->parent);
  
  EXPECT_FALSE(syms.scopes[0]->table.empty());
  EXPECT_TRUE(syms.scopes[1]->table.empty());
  EXPECT_TRUE(syms.scopes[2]->table.empty());
}

TEST_F(Modules, Regular) {
  ASTs asts;
  asts.push_back(makeModuleAST("a", {"b", "c"}));
  asts.push_back(makeModuleAST("b", {"d", "e"}));
  asts.push_back(makeModuleAST("c", {}));
  asts.push_back(makeModuleAST("d", {"c"}));
  asts.push_back(makeModuleAST("e", {"d"}));
  asts.push_back(makeModuleAST("f", {"d", "e"}));
  const ModuleOrder order = findModuleOrder(asts, log());
  EXPECT_EQ(order.size(), 6);
  EXPECT_EQ(order[0], 2); // c
  EXPECT_EQ(order[1], 3); // d
  EXPECT_EQ(order[2], 4); // e
  EXPECT_EQ(order[3], 1); // b
  EXPECT_EQ(order[4], 0); // a
  EXPECT_EQ(order[5], 5); // f
}

TEST_F(Modules, No_imports) {
  ASTs asts;
  asts.push_back(makeModuleAST("a", {}));
  asts.push_back(makeModuleAST("b", {}));
  asts.push_back(makeModuleAST("c", {}));
  asts.push_back(makeModuleAST("d", {}));
  asts.push_back(makeModuleAST("e", {}));
  asts.push_back(makeModuleAST("f", {}));
  const ModuleOrder order = findModuleOrder(asts, log());
  EXPECT_EQ(order.size(), 6);
  EXPECT_EQ(order[0], 0); // a
  EXPECT_EQ(order[1], 1); // b
  EXPECT_EQ(order[2], 2); // c
  EXPECT_EQ(order[3], 3); // d
  EXPECT_EQ(order[4], 4); // e
  EXPECT_EQ(order[5], 5); // f
}

TEST_F(Modules, Invalid_import) {
  ASTs asts;
  asts.push_back(makeModuleAST("a", {}));
  asts.push_back(makeModuleAST("b", {"aye"}));
  EXPECT_THROW(findModuleOrder(asts, log()), FatalError);
}

TEST_F(Modules, Cycle) {
  ASTs asts;
  asts.push_back(makeModuleAST("a", {"b"}));
  asts.push_back(makeModuleAST("b", {"e"}));
  asts.push_back(makeModuleAST("c", {"d"}));
  asts.push_back(makeModuleAST("d", {"e"}));
  asts.push_back(makeModuleAST("e", {"c"}));
  EXPECT_THROW(findModuleOrder(asts, log()), FatalError);
}

TEST_F(Modules, Self_import) {
  ASTs asts;
  asts.push_back(makeModuleAST("a", {"a"}));
  EXPECT_THROW(findModuleOrder(asts, log()), FatalError);
}

TEST_F(Modules, None) {
  EXPECT_TRUE(findModuleOrder({}, log()).empty());
}

TEST_F(Modules, Import_external) {
  ASTs asts;
  asts.push_back(makeModuleAST("a", {"external_a", "b"}));
  asts.push_back(makeModuleAST("b", {"external_b"}));
  ast::Names external;
  external.push_back("external_a");
  external.push_back("external_b");
  const ModuleOrder order = findModuleOrder(asts, external, log());
  EXPECT_EQ(order.size(), 2);
  EXPECT_EQ(order[0], 1); // b
  EXPECT_EQ(order[1], 0); // a
}

TEST_F(Modules, Duplicate) {
  ASTs asts;
  asts.push_back(makeModuleAST("a", {}));
  ast::Names external;
  external.push_back("a");
  EXPECT_THROW(findModuleOrder(asts, external, log()), FatalError);
}

TEST_F(Modules, Import_type) {
  const char *sourceA = R"(
    module ModA;
  
    type Number = real;
    type SpecialNumber = Number;
  )";
  
  const char *sourceB = R"(
    module ModB;
  
    import ModA;
  
    type CoolNumber = SpecialNumber;
    type GreatNumber = CoolNumber;
  
    func main() {
      let n: GreatNumber = 5.0;
      let r: real = n;
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(sourceB, log()));
  asts.push_back(createAST(sourceA, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST_F(Modules, Import_constant) {
  const char *sourceA = R"(
    module ModA;
  
    type Number = real;
  
    let five: Number = 5.0;
  )";
  
  const char *sourceB = R"(
    module ModB;
  
    import ModA;
  
    func main() {
      let n: real = five;
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(sourceB, log()));
  asts.push_back(createAST(sourceA, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST_F(Modules, Import_function) {
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
      let n: real = getFive();
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(sourceB, log()));
  asts.push_back(createAST(sourceA, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST_F(Modules, Import_method) {
  const char *sourceA = R"(
    module ModA;
  
    func (self: real) half() -> real {
      return self / 2.0;
    }
  )";
  
  const char *sourceB = R"(
    module ModB;
  
    import ModA;
  
    func main() {
      let fourteen = 14.0;
      let seven = fourteen.half();
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(sourceB, log()));
  asts.push_back(createAST(sourceA, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST_F(Modules, Import_struct) {
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
      let vec: Vec2 = getVec2();
      let x: Number = vec.x;
      let y: real = vec.y;
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(sourceB, log()));
  asts.push_back(createAST(sourceA, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST_F(Modules, Forgot_to_import) {
  const char *sourceA = R"(
    module ModA;
  
    type Number = Number;
  )";
  
  const char *sourceB = R"(
    module ModB;
  
    import ModA;
  
    type Number = real;
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(sourceB, log()));
  asts.push_back(createAST(sourceA, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  EXPECT_THROW(compileModules(syms, order, asts, log()), FatalError);
}

TEST_F(Modules, Shadowing) {
  const char *sourceA = R"(
    module ModA;
  
    type Number = real;
    func fn() -> real {
      return 0.0;
    }
    let const = 0.0;
  )";
  
  const char *sourceB = R"(
    module ModB;
  
    import ModA;
  
    type Number = sint;
    func fn() -> sint {
      return 0;
    }
    let const = 0;
  
    func test() {
      let a: sint = make Number {};
      let b: sint = fn();
      let c: sint = const;
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(sourceB, log()));
  asts.push_back(createAST(sourceA, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST_F(Modules, Cmath) {
  const char *source = R"(
    import cmath;
  
    type Vec2 struct {
      x: real;
      y: real;
    };
  
    func (self: Vec2) mag() {
      return sqrt(self.x*self.x + self.y*self.y);
    }
  
    func test() {
      let vec = make Vec2 {3.0, 4.0};
      let five = vec.mag();
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  asts.push_back(createAST(source, log()));
  asts.push_back(makeCmath(syms.builtins, log()));
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST(Func, Redef) {
  EXPECT_FAILS(R"(
    func myFunction() {
      
    }
    func myFunction() {
      
    }
  )");
}

TEST(Func, Redef_with_params) {
  EXPECT_FAILS(R"(
    func myFunction(i: sint, f: real) {
      
    }
    func myFunction(i: sint, f: real) {
      
    }
  )");
}

TEST(Func, Redef_weak_alias) {
  EXPECT_FAILS(R"(
    type Number = sint;
    func myFunction(i: Number) {
      
    }
    func myFunction(i: sint) {
      
    }
  )");
}

TEST(Func, Redef_multi_weak_alias) {
  EXPECT_FAILS(R"(
    type Integer = sint;
    type Number = Integer;
    type SpecialNumber = Number;
    type TheNumber = SpecialNumber;
    func myFunction(i: TheNumber) {
      
    }
    func myFunction(i: sint) {
      
    }
  )");
}

TEST(Func, Redef_strong_alias) {
  EXPECT_FAILS(R"(
    type Integer = sint;
    type Number Integer;
    type SpecialNumber = Number;
    type TheNumber = SpecialNumber;
    func myFunction(i: TheNumber) {
      
    }
    func myFunction(i: Number) {
      
    }
  )");
}

TEST(Func, Overload_strong_alias) {
  EXPECT_SUCCEEDS(R"(
    type Number real;
    type Float real;
    func myFunction(i: Number) {
      
    }
    func myFunction(i: Float) {
      
    }
  )");
}

TEST(Func, Overloading) {
  EXPECT_SUCCEEDS(R"(
    func myFunction(n: real) {
      
    }
    func myFunction(n: sint) {
      
    }
  )");
}

TEST(Func, Tag_dispatch) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Func, Discarded_return) {
  EXPECT_FAILS(R"(
    func myFunction() -> sint {
      return 5;
    }
  
    func main() {
      myFunction();
    }
  )");
}

TEST(Func, Recursive) {
  EXPECT_SUCCEEDS(R"(
    func fac(n: uint) -> uint {
      return n == 0u ? 1u : n * fac(n - 1u);
    }
  )");
}

TEST(Sym, Undefined) {
  EXPECT_FAILS(R"(
    func myFunction(i: Number) {
      
    }
    func myFunction(i: sint) {
      
    }
  )");
}

TEST(Sym, Colliding_type_and_func) {
  EXPECT_FAILS(R"(
    type fn func();
    func fn() {}
  )");
}

TEST(Sym, Type_in_func) {
  EXPECT_SUCCEEDS(R"(
    func fn() {
      type Number = real;
      let n: Number = 4.3;
    }
  )");
}

TEST(Var, Var_without_type_or_value) {
  EXPECT_FAILS(R"(
    var a;
  )");
}

TEST(Var, Redefine_var) {
  EXPECT_FAILS(R"(
    var x: sint;
    var x: real;
  )");
}

TEST(Var, Redefine_let) {
  EXPECT_FAILS(R"(
    let x: sint = 0;
    let x: real = 0.0;
  )");
}

TEST(Var, Type_mismatch) {
  EXPECT_FAILS(R"(
    let x: sint = 2.5;
  )");
}

TEST(Var, Recursive) {
  EXPECT_FAILS(R"(
    let x = x;
  )");
}

TEST(Type, Recursive) {
  EXPECT_FAILS(R"(
    type x = x;
  )");
}

TEST(Struct, Recursive) {
  EXPECT_FAILS(R"(
    type Struct struct {
      x: Struct;
    };
  )");
}

TEST(Struct, Enum) {
  EXPECT_SUCCEEDS(R"(
    type Dir sint;
    let Dir_up    = make Dir 0;
    let Dir_right = make Dir 1;
    let Dir_down  = make Dir 2;
    let Dir_left  = make Dir 3;
  )");
}

TEST(Struct, Main) {
  EXPECT_SUCCEEDS(R"(
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
      
    func (self: ref Vec) add(other: Vec) {
      self.x += other.x;
      self.y += other.y;
    }
  
    func (self: ref Vec) div(val: real) {
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
  )");
}

TEST(Struct, More) {
  EXPECT_SUCCEEDS(R"(
    type MyString struct {
      s: [char];
    };
  
    func change(string: ref [char]) {
      string = "still a string";
    }
  
    func change(mstr: ref MyString) {
      change(mstr.s);
    }
  
    func main() {
      var thing: MyString;
      thing.s = "a string";
      change(thing);
    }
  )");
}

TEST(Struct, Dup_method) {
  EXPECT_FAILS(R"(
    type MyStruct struct {};
  
    func (self: MyStruct) fn() {}
    func (self: MyStruct) fn() {}
  )");
}

TEST(Struct, Member_function_sint) {
  EXPECT_SUCCEEDS(R"(
    func (self: ref sint) zero() {
      self = 0;
    }
  
    func test() {
      var n = 5;
      n.zero();
    }
  )");
}

TEST(Struct, Colliding_func_field) {
  EXPECT_FAILS(R"(
    type MyStruct struct {
      fn: sint;
    };
  
    func (self: MyStruct) fn() {}
  )");
}

TEST(Struct, Dup_field) {
  EXPECT_FAILS(R"(
    type MyStruct struct {
      m: sint;
      m: real;
    };
  )");
}

TEST(Struct, Access_undef_field) {
  EXPECT_FAILS(R"(
    type MyStruct struct {
      ajax: sint;
    };
  
    func oh_no() {
      var s: MyStruct;
      s.francis = 4;
    }
  )");
}

TEST(Value_cat, Swap_literals) {
  EXPECT_FAILS(R"(
    func swap(a: ref sint, b: ref sint) {
      let temp: sint = a;
      a = b;
      b = temp;
    }
  
    func main() {
      swap(4, 5);
    }
  )");
}

TEST(Value_cat, Swap_constants) {
  EXPECT_FAILS(R"(
    func swap(a: ref sint, b: ref sint) {
      let temp: sint = a;
      a = b;
      b = temp;
    }
  
    func main() {
      let four = 4;
      let five = 5;
      swap(four, five);
    }
  )");
}

TEST(Value_cat, Assign_to_literal) {
  EXPECT_FAILS(R"(
    func main() {
      '4' = '5';
    }
  )");
}

TEST(Value_cat, Assign_to_ternary) {
  EXPECT_SUCCEEDS(R"(
    func main() {
      var a = 5;
      {
        var b = 7;
        {
          (a < b ? a : b) = (a > b ? a : b);
        }
      }
    }
  )");
}

TEST(Value_cat, Non_bool_ternary_condition) {
  EXPECT_FAILS(R"(
    func main() {
      let test = ("nope" ? 4 : 6);
    }
  )");
}

TEST(Expr, Ternary_condition_type_mismatch) {
  EXPECT_FAILS(R"(
    func main() {
      let test = (true ? 4 : "nope");
    }
  )");
}

TEST(Expr, Assign_type) {
  EXPECT_FAILS(R"(
    type Type struct {};
  
    func main() {
      let t = Type;
    }
  )");
}

TEST(Expr, Returning_an_object) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Expr, Type_in_function) {
  EXPECT_SUCCEEDS(R"(
    func fn() -> sint {
      type MyStruct struct {
        x: sint;
      };
      let nine = make MyStruct {9};
      return nine.x;
    }
  )");
}

TEST(Expr, Access_type_in_function) {
  EXPECT_FAILS(R"(
    func fn() {
      type Thing sint;
    }
  
    let thing = make Thing 1;
  )");
}

TEST(Expr, Expected_type) {
  EXPECT_FAILS(R"(
    let not_a_type = 4;
    var oops: not_a_type;
  )");
}

TEST(Expr, Must_call_mem_func) {
  EXPECT_FAILS(R"(
    type Struct struct {};
  
    func (self: Struct) fn() {}
  
    var s: Struct;
    let test = s.fn;
  )");
}

TEST(If, Standard) {
  EXPECT_SUCCEEDS(R"(
    func main() {
      var a = 4;
      var b = 4;
      if (a == b) {
        a += b;
      }
    }
  )");
}

TEST(If, Else) {
  EXPECT_SUCCEEDS(R"(
    func main() {
      var a = 4;
      var b = 4;
      if (a == b) {
        a += b;
      } else if (a < b) {
        b -= a;
      }
    }
  )");
}

TEST(If, Non_bool) {
  EXPECT_FAILS(R"(
    func main() {
      var a = 4;
      if (a) {
        a = 0;
      }
    }
  )");
}

TEST(If, Scope) {
  EXPECT_FAILS(R"(
    func main() {
      if (true) var i = 3;
      i = 2;
    }
  )");
}

TEST(Loops, All) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Loops, For_var_scope) {
  EXPECT_FAILS(R"(
    func main() {
      for (i := 0; i != 10; i++) {}
      i = 3;
    }
  )");
}

TEST(Loops, Break_outside_loop) {
  EXPECT_FAILS(R"(
    func main() {
      break;
    }
  )");
}

TEST(Switch, Standard) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Switch, Multiple_default) {
  EXPECT_FAILS(R"(
    func main() {
      let num = 3;
      switch (num) {
        default {}
        case (3) {}
        default {}
      }
    }
  )");
}

TEST(Switch, Type_mismatch) {
  EXPECT_FAILS(R"(
    func main() {
      let num = 3;
      switch (num) {
        default {}
        case ("oops") {}
      }
    }
  )");
}

TEST(Expr, CompAssign_to_const) {
  EXPECT_FAILS(R"(
    func main() {
      let constant = 0;
      constant += 4;
    }
  )");
}

TEST(Expr, CompAssign_bitwise_float) {
  EXPECT_FAILS(R"(
    func main() {
      let float = 2.0;
      float |= 4.0;
    }
  )");
}

TEST(Expr, Increment_struct) {
  EXPECT_FAILS(R"(
    func main() {
      var strut: struct{};
      strut++;
    }
  )");
}

TEST(Expr, Assign_diff_types) {
  EXPECT_FAILS(R"(
    func main() {
      var num = 5;
      var str = "5";
      num = str;
    }
  )");
}

TEST(Expr, Unary_operators) {
  EXPECT_SUCCEEDS(R"(
    func main() {
      let test0 = -(5);
      let test1 = -(5.0);
      let test2 = -('5');
      let test3 = ~(5u);
      let test4 = !(false);
    }
  )");
}

TEST(Expr, Bitwise_ops) {
  EXPECT_SUCCEEDS(R"(
    func main() {
      let five = 5u;
      let ten = five << 1u;
      let two = ten & 2u;
      let t = true;
      let fols0 = ten < two;
      let fols1 = t && fols0;
    }
  )");
}

TEST(Expr, Add_float_int) {
  EXPECT_FAILS(R"(
    func main() {
      let f = 3.0;
      let i = 3;
      let sum = f + i;
    }
  )");
}

TEST(Expr, Bitwise_not_float) {
  EXPECT_FAILS(R"(
    func main() {
      let test = ~(3.0);
    }
  )");
}

TEST(Void, Storing_void) {
  EXPECT_FAILS(R"(
    func nothing() {}
  
    func main() {
      let void = nothing();
    }
  )");
}

TEST(Void, Access_field_on_void) {
  EXPECT_FAILS(R"(
    func nothing() {}
  
    func main() {
      let value = nothing().field;
    }
  )");
}

TEST(Void, Member_function) {
  EXPECT_FAILS(R"(
    func getVoid() {}
  
    func memFun(a: sint) -> real {
      return 5.0;
    }
  
    func test() {
      let five: real = getVoid().memFun(5);
    }
  )");
}

TEST(Expr, Access_field_on_int) {
  EXPECT_FAILS(R"(
    func integer() -> sint {
      return 0;
    }
  
    func main() {
      let value = integer().field;
    }
  )");
}

TEST(Expr, Call_a_float) {
  EXPECT_FAILS(R"(
    func main() {
      let fl = 3.0;
      fl(5.0);
    }
  )");
}

TEST(Literals, String) {
  EXPECT_SUCCEEDS(R"(
    let str: [char] = "This is a string";
    let empty: [char] = "";
    let escapes: [char] = "\'\"\?\\\a\b\f\n\r\t\v\00000000000011\x000011";
  )");
}

TEST(Literals, Oct_range) {
  EXPECT_FAILS(R"(
    let str: [char] = "\7777777";
  )");
}

TEST(Literals, Hex_range) {
  EXPECT_FAILS(R"(
    let str: [char] = "\xfffffff";
  )");
}

TEST(Literals, Char) {
  EXPECT_SUCCEEDS(R"(
    let a: char = 'a';
    let newline: char = '\n';
    let newline1: char = '\x0a';
    let zero: char = '\0';
  )");
}

TEST(Literals, Empty_char) {
  EXPECT_FAILS(R"(
    let c: char = '';
  )");
}

TEST(Literals, Long_char) {
  EXPECT_FAILS(R"(
    let c: char = 'hello';
  )");
}

TEST(Literals, Number) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Literals, Bool) {
  EXPECT_SUCCEEDS(R"(
    let t: bool = true;
    let f: bool = false;
  )");
}

TEST(Literals, Array) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Literals, Array_to_sint) {
  EXPECT_FAILS(R"(
    let number: sint = [1, 2, 3];
  )");
}

TEST(Literals, Empty_array) {
  EXPECT_FAILS(R"(
    let empty = [];
  )");
}

TEST(Literals, Elem_diff_to_array_type) {
  EXPECT_FAILS(R"(
    type StrongSint sint;
    let arr: [StrongSint] = [1, make StrongSint 2];
  )");
}

TEST(Literals, Diff_types_in_array) {
  EXPECT_FAILS(R"(
    type StrongSint sint;
    let arr = [1, make StrongSint 2];
  )");
}

TEST(Subscript, Regular) {
  EXPECT_SUCCEEDS(R"(
    type real_array = [real];
    let arr: real_array = [4.2, 11.9, 1.5];
    let second: real = arr[1];
    let third = arr[2];
  
    type index uint;
    let first: real = arr[make index 0];
  )");
}

TEST(Subscript, Strong_array) {
  EXPECT_SUCCEEDS(R"(
    type strong_array [sint];
    let arr = make strong_array [8, 2, 3];
    let two = arr[1];
  )");
}

TEST(Subscript, Invalid_index) {
  EXPECT_FAILS(R"(
    let arr: [bool] = [false, true];
    let nope: bool = arr[1.0];
  )");
}

TEST(Subscript, Struct) {
  EXPECT_FAILS(R"(
    var not_array: struct{};
    let nope = not_array[0];
  )");
}

TEST(Make, Init_list) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Make, Five_to_struct) {
  EXPECT_FAILS(R"(
    let thing = make struct{} 5;
  )");
}

TEST(Make, Cast_strong_structs) {
  EXPECT_SUCCEEDS(R"(
    type ToughStruct struct {
      value: sint;
    };
    type StrongBoi struct {
      value: sint;
    };
    let strong = make StrongBoi make ToughStruct {};
  )");
}

TEST(Make, Cast_strong_structs_diff_names) {
  EXPECT_FAILS(R"(
    type ToughStruct struct {
      val: sint;
    };
    type StrongBoi struct {
      value: sint;
    };
    let strong = make StrongBoi make ToughStruct {};
  )");
}

TEST(Init_list, No_type) {
  EXPECT_FAILS(R"(
    let anything = {};
  )");
}

TEST(Init_list, Init_sint) {
  EXPECT_FAILS(R"(
    let number: sint = {5};
  )");
}

TEST(Init_list, Too_many_expr) {
  EXPECT_FAILS(R"(
    type One struct {
      val: sint;
    };
    let two: One = {1, 2};
  )");
}

TEST(Init_list, Too_few_expr) {
  EXPECT_FAILS(R"(
    type Two struct {
      a: sint;
      b: sint;
    };
    let one: Two = {1};
  )");
}

TEST(Init_list, Type_mismatch) {
  EXPECT_FAILS(R"(
    type Number real;
    type Struct struct {
      a: Number;
    };
    let s: Struct = {5.0};
  )");
}

TEST(Return, Sint_to_real) {
  EXPECT_FAILS(R"(
    func getReal() -> real {
      return 91;
    }
  )");
}

TEST(Return, Concrete_to_strong) {
  EXPECT_FAILS(R"(
    type StrongEmpty struct{};
  
    func getStrong() -> StrongEmpty {
      return make struct{} {};
    }
  )");
}

TEST(Return, Void_expression) {
  EXPECT_SUCCEEDS(R"(
    func getVoid() {}
    func getVoidAgain() {
      return getVoid();
    }
  )");
}

TEST(Func_addr, Single) {
  EXPECT_SUCCEEDS(R"(
    type Vec2 struct {
      x: real;
      y: real;
    };
    type Vector = Vec2;
  
    func one(a: real, b: ref real, c: Vector) -> Vec2 {
      b = a;
      return make Vector {c.x * a, c.y + a};
    }
  
    let ptr0 = one;
    let ptr1: func(real, ref real, Vector) -> Vec2 = one;
    let ptr2: func(real, ref real, Vec2) -> Vector = one;
    let ptr3 = make func(real, ref real, Vector) -> Vec2 one;
  )");
}

TEST(Func_addr, Overloaded) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Func_addr, Single_method) {
  EXPECT_FAILS(R"(
    func (self: sint) memfun(param: real) -> real {
      return make real self * param;
    }
  
    let ptr = memfun;
  )");
}

TEST(Func_addr, Overloaded_method) {
  EXPECT_FAILS(R"(
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
  )");
}

TEST(Func_addr, Ambiguous_overloaded) {
  EXPECT_FAILS(R"(
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
  )");
}

TEST(Func_addr, Single_no_match_signature) {
  EXPECT_FAILS(R"(
    func add(a: sint, b: sint) -> sint {
      return a + b;
    }
  
    let ptr: func(real, sint) -> sint = add;
  )");
}

TEST(Func_addr, Overloaded_no_match_signature) {
  EXPECT_FAILS(R"(
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
  )");
}

TEST(Func_addr, Void_return) {
  EXPECT_SUCCEEDS(R"(
    func getVoid(a: sint) {}
  
    let ptr: func(sint) = getVoid;
  )");
}

TEST(Expr, Compare_anything) {
  EXPECT_SUCCEEDS(R"(
    func arrays(a: [bool], b: [bool]) -> bool {
      return a < b;
    }
    
    func functions(a: func(sint), b: func(sint)) -> bool {
      return a < b;
    }
    
    func structs(a: struct {m: sint;}, b: struct {m: sint;}) -> bool {
      return a < b;
    }
  )");
}

TEST(Func, Undefined_method) {
  EXPECT_FAILS(R"(
    func test() {
      let x = 10;
      x.undefined();
    }
  )");
}

TEST(Sym, Undefined_symbol) {
  EXPECT_FAILS(R"(
    func test() {
      let x = 10;
      x = undefined;
    }
  )");
}

TEST(Sym, Calling_type) {
  EXPECT_FAILS(R"(
    type MyStruct struct {};
    func test() {
      // this looks like a function call
      // that's why I chose the make syntax
      let instance = MyStruct();
    }
  )");
}

TEST(Value_cat, Assign_to_paramenter) {
  EXPECT_SUCCEEDS(R"(
    func fn(a: sint, b: ref sint) {
      a = 1;
      b = 2;
    }
  )");
}

TEST(Lambda, Regular) {
  EXPECT_SUCCEEDS(R"(
    func giveMeFunction(fn: func(real)->sint) {
      let result: sint = fn(2.0);
    }
  
    func notLambda(a: real) -> sint {
      return 11 * make sint a;
    }
  
    func test() {
      giveMeFunction(func(a: real) -> sint {
        return 11 * make sint a;
      });
      giveMeFunction(notLambda);
    }
  )");
}

TEST(Lambda, Immediately_invoked) {
  EXPECT_SUCCEEDS(R"(
    let nine = func() -> sint {
      return 4 + 5;
    }();
  )");
}

TEST(Lambda, Wrong_args) {
  EXPECT_FAILS(R"(
    func test() {
      let lam = func(a: sint) -> sint {
        return a * 2;
      };
      let double = lam(16.3);
    }
  )");
}

TEST(Lambda, Unused_return) {
  EXPECT_FAILS(R"(
    func test() {
      let identity = func(a: sint) -> sint {
        return a;
      };
      identity(11);
    }
  )");
}

TEST(Lambda, Partial_application) {
  EXPECT_SUCCEEDS(R"(
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
  )");
}

TEST(Lambda, Stateful) {
  EXPECT_SUCCEEDS(R"(
    func makeIDgen(first: sint) -> func() -> sint {
      let id = first - 1;
      return func() -> sint {
        id++;
        return id;
      };
    }

    func test() {
      let gen = makeIDgen(4);
      let four = gen();
      let five = gen();
      let six = gen();
    }
  )");
}

TEST(Value_cat, Increment_constant) {
  EXPECT_FAILS(R"(
    func test() {
      let one = 1;
      one++;
    }
  )");
}

TEST(Expr, Compound_assign_different_types) {
  EXPECT_FAILS(R"(
    func test() {
      var dst = 8;
      dst *= 2.0;
    }
  )");
}

TEST(Func, Nested_function) {
  EXPECT_FAILS(R"(
    func outer() {
      func inner() {}
    }
  )");
}

TEST(Type, Invalid_array_elem) {
  EXPECT_FAILS(R"(
    var yeah: [[not_a_type]];
  )");
}

TEST(Type, Validate_huge_type) {
  EXPECT_SUCCEEDS(R"(
    type Number real;
    type Type func();
  
    type LargeType [struct {
      field: real;
      fn: func(sint, Number) -> Type;
    }];
    type AnotherType = LargeType;
  )");
}

TEST(Type, Invalid_return_type_deep) {
  EXPECT_FAILS(R"(
    type Number real;
    type Type func();
  
    type LargeType [struct {
      field: real;
      fn: func(sint, Number) -> MyType;
    }];
    type AnotherType = LargeType;
  )");
}

TEST(Shadow, Variable_shadow_with_global) {
  EXPECT_SUCCEEDS(R"(
    let zero = 0;
    
    func fn() {
      let zero = 11;
    }
  )");
}

TEST(Shadow, Type_shadow_within_function) {
  EXPECT_SUCCEEDS(R"(
    func fn() {
      type t sint;
      {
        type t real;
      }
    }
  )");
}

TEST(Shadow, Variable_shadow_with_builtin_function) {
  EXPECT_SUCCEEDS(R"(
    let size = 4;
  )");
}

TEST(Expect_type, Empty_struct_to_sint_array) {
  EXPECT_FAILS(R"(
    let test = make [sint] make struct{} {};
  )");
}

TEST(Expect_type, Expected_sint_array_got_empty_struct) {
  EXPECT_FAILS(R"(
    let test: [sint] = make struct{} {};
  )");
}

TEST(Expect_type, Cast_struct_to_strong_array) {
  EXPECT_FAILS(R"(
    type Vec2 struct {
      x: real;
      y: real;
    };
    type Numbers [sint];
    let test = make Numbers make Vec2 {};
  )");
}

TEST(Expect_type, Struct_strong_array) {
  EXPECT_FAILS(R"(
    type Vec2 struct {
      x: real;
      y: real;
    };
    type Numbers [sint];
    let test: Numbers = make Vec2 {};
  )");
}

TEST(Expr, Add_different_strong_types) {
  EXPECT_FAILS(R"(
    type Number sint;
    
    func test(a: Number, b: sint) {
      return a + b;
    }
  )");
}

TEST(Return, Void_function) {
  EXPECT_SUCCEEDS(R"(
    func getVoid() {
      return;
    }
    
    func getVoidAgain() {
      return getVoid();
    }
  )");
}

TEST(Return, Void_to_sint) {
  EXPECT_FAILS(R"(
    func getSint() -> sint {
      return;
    }
  )");
}

TEST(Bool_conv, Functor) {
  EXPECT_SUCCEEDS(R"(
    func test() {
      var nullFn: func();
      if (nullFn) {}
      for (;nullFn;) {}
      while (nullFn) {}
      let two = nullFn ? 6 : 2;
      let troo = !make bool nullFn;
      let tru = !nullFn;
      let fls = true && nullFn;
      let fols = false || nullFn;
      let fals: bool = nullFn;
    }
  )");
}

TEST(Func_addr, Builtin) {
  EXPECT_FAILS(R"(
    let ptr = push_back;
  )");
}

TEST(Btn_func, Regular) {
  EXPECT_SUCCEEDS(R"(
    func test() {
      var array: [sint] = [1, 4, 9, 16, 25, 36];
      var otherArray: [sint] = array;
      let eight: uint = capacity(array);
      let six: uint = size(otherArray);
      push_back(otherArray, make sint eight);
      append(array, [49, 64, 81, 100]);
      pop_back(otherArray);
      resize(otherArray, size(array));
      reserve(otherArray, capacity(otherArray) * six);
    }
  )");
}

TEST(Btn_func, Too_many_args) {
  EXPECT_FAILS(R"(
    let test = size(4, 5, 1, 6, 2);
  )");
}

TEST(Btn_func, Expected_array) {
  EXPECT_FAILS(R"(
    let test = size(0);
  )");
}

TEST(Btn_func, Expected_uint) {
  EXPECT_FAILS(R"(
    func test() {
      var array: [sint] = [];
      resize(array, 55);
    }
  )");
}

TEST(Btn_func, Constant_array) {
  EXPECT_FAILS(R"(
    func test() {
      let array: [sint] = [];
      resize(array, 55u);
    }
  )");
}

TEST(Btn_func, Bad_argument_to_push_back) {
  EXPECT_FAILS(R"(
    func test() {
      var array: [sint] = [];
      push_back(array, make struct{too: real;} {}
    }
  )");
}

TEST(Btn_func, Bad_argument_to_append) {
  EXPECT_FAILS(R"(
    func test() {
      var array: [sint] = [];
      append(array, 5);
    }
  )");
}

TEST(Btn_func, Squares_array) {
  EXPECT_SUCCEEDS(R"(
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
      let empty = squares(0u);
      let one_four_nine = squares(3u);
    }
  )");
}

TEST(Btn_func, Int_stack) {
  EXPECT_SUCCEEDS(R"(
    type IntStack [sint];

    func (self: ref IntStack) push(value: sint) {
      push_back(self, value);
    }
    func (self: ref IntStack) pop() {
      pop_back(self);
    }
    func (self: ref IntStack) pop_top() -> sint {
      let top = self[size(self) - 1u];
      pop_back(self);
      return top;
    }
    func (self: IntStack) top() -> sint {
      return self[size(self) - 1u];
    }
    func (self: IntStack) empty() -> bool {
      return size(self) == 0u;
    }
  )");
}

TEST(Expr, Bad_call_to_method) {
  EXPECT_FAILS(R"(
    let thing = 11;
    let val = make struct{} {};
    let result = val.thing();
  )");
}

TEST(Lambda, Void_return) {
  EXPECT_SUCCEEDS(R"(
    func test() {
      let lam = func(a: ref sint) {
        a += 4;
      };
      var yeah = 7;
      lam(yeah);
    }
  )");
}

TEST(Shadow, Struct_field_type) {
  EXPECT_SUCCEEDS(R"(
    type t = sint;
    func test() {
      var sintStruct: struct {m: t;};
      type t = real;
      var anotherSintStruct: struct {m: sint;};
      sintStruct = anotherSintStruct;
    }
  )");
}

TEST(Func, Return_type_deduction) {
  EXPECT_SUCCEEDS(R"(
    func fac(n: sint) {
      if (n == 0) {
        return 1;
      } else {
        return n * fac(n - 1);
      }
    }
    
    func oneOrNone(b: bool) {
      if (b) {
        return [1];
      } else {
        return [];
      }
    }
    
    func test() {
      let two: sint = fac(2);
      let six = fac(3);
      let none = oneOrNone(false);
      let one: [sint] = oneOrNone(true);
    }
  )");
}

TEST(Func, Return_type_cannot_be_deduced) {
  EXPECT_FAILS(R"(
    func fac(n: sint) {
      return n == 0 ? 1 : n * fac(n - 1);
    }
  )");
}

TEST(Lambda, Return_type_deduction) {
  EXPECT_SUCCEEDS(R"(
    func makeAdd(a: sint) {
      return func(b: sint) {
        return func(c: sint) {
          return a + b + c;
        };
      };
    }
    
    let add_111 = makeAdd(111);
    let add_222: func(sint) -> func(sint) -> sint = makeAdd(222);
    
    let five = func() { return 5; }();
    let six: real = func() { return 6.0; }();
  )");
}

TEST(Void, Array) {
  EXPECT_FAILS(R"(
    func nothing() {}
    let array = [nothing()];
  )");
}

TEST(Void, Cast_from_void) {
  EXPECT_FAILS(R"(
    func nothing() {}
    let something = make sint nothing();
  )");
}

TEST(Value_cat, Assign_to_function_return) {
  EXPECT_FAILS(R"(
    func getFive() {
      return 5;
    }
    
    func test() {
      getFive() = 11;
    }
  )");
}

TEST(Value_cat, Assign_to_lambda_return) {
  EXPECT_FAILS(R"(
    let getFive = func() {
      return 5;
    };
    
    func test() {
      getFive() = 11;
    }
  )");
}

TEST(Void, Compare) {
  EXPECT_FAILS(R"(
    func getVoid() {}
    
    func test() {
      return getVoid() == getVoid();
    }
  )");
}

TEST(Flow, Missing_return) {
  EXPECT_FAILS(R"(
    func getSomething() -> sint {}
  )");
}

TEST(Flow, No_return_from_void_function) {
  EXPECT_SUCCEEDS(R"(
    func test(x: bool) {
      if (x) {
        return;
      }
    }
  )");
}

TEST(Flow, May_not_return) {
  EXPECT_FAILS(R"(
    func maybe3(b: bool) -> sint {
      if (b) {
        return 3;
      }
    }
  )");
}

TEST(Flow, If_will_return) {
  EXPECT_SUCCEEDS(R"(
    func willReturn(b: bool) -> sint {
      if (b) {
        return 3;
      } else {
        return -1;
      }
    }
  )");
}

TEST(Flow, Continue_to_nothing) {
  EXPECT_FAILS(R"(
    func test(x: sint) {
      switch (x) {
        case (5) {
          continue;
        }
      }
    }
  )");
}

TEST(Flow, Unreachable) {
  EXPECT_FAILS(R"(
    func test() {
      if (true) {
        return;
      } else {
        return;
      }
      var a = 1;
    }
  )");
}

TEST(Flow, Switch_will_return) {
  EXPECT_SUCCEEDS(R"(
    func test(x: sint) -> real {
      switch (x) {
        case (5) {
          return 2.0;
        }
        default {
          continue;
        }
        case (15) {
          return 4.9;
        }
      }
    }
  )");
}

TEST(Flow, Switch_may_not_return) {
  EXPECT_FAILS(R"(
    func test(x: sint) -> real {
      switch (x) {
        case (5) {
          return 2.0;
        }
        default {
          continue;
        }
        case (15) {
          break;
        }
      }
    }
  )");
}

TEST(Flow, Switch_no_return) {
  EXPECT_FAILS(R"(
    let test = func(x: sint) -> real {
      switch (x) {
        case (5) {
          break;
        }
        default {
          continue;
        }
        case (15) {
          break;
        }
      }
    };
  )");
}

TEST(Flow, Return_after_switch) {
  EXPECT_SUCCEEDS(R"(
    let test = func(x: sint) -> real {
      switch (x) {
        case (5) {
          break;
        }
        default {
          continue;
        }
        case (15) {
          return 4.9;
        }
      }
      return 2.0;
    };
  )");
}

TEST(Flow, For_loop_may_not_return) {
  EXPECT_FAILS(R"(
    let test = func(x: sint) -> real {
      for (i := x; i != x + 10; i++) {
        return 2.0;
      }
    };
  )");
}

TEST(Flow, While_loop_may_not_return) {
  EXPECT_FAILS(R"(
    let test = func(x: sint) -> real {
      while (x != 4) {
        return 2.0;
      }
    };
  )");
}

TEST(Flow, Switch_may_not_return_after_break) {
  EXPECT_FAILS(R"(
    let test = func(x: sint) -> real {
      switch (x) {
        case (5) {
          if (x % 2 == 0) {
            break;
          }
          return 2.0;
        }
        default {
          return 4.9;
        }
      }
    };
  )");
}

TEST(Flow, Maybe_always_return) {
  EXPECT_SUCCEEDS(R"(
    let test = func(x: sint) -> real {
      if (x == 7) {
        return 2.0;
      }
      return 4.9;
    };
  )");
}

TEST(Flow, Maybe_break_maybe_return) {
  EXPECT_FAILS(R"(
    let test = func(x: sint) -> real {
      switch (x) {
        case (5) {
          if (x % 2 == 0) {
            break;
          }
          if (true) {
            return 2.0;
          }
        }
        default {
          return 4.9;
        }
      }
    };
  )");
}

TEST(Flow, Empty_switch_no_return) {
  EXPECT_FAILS(R"(
    func test(x: sint) -> real {
      switch (x) {}
    }
  )");
}

retain_ptr<ast::ExtFunc> makeSqrt(sym::Builtins &btn) {
  auto sqrt = make_retain<ast::ExtFunc>();
  sqrt->name = "sqrt";
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
  )";

  Symbols syms = initModules(log());
  ASTs asts;
  
  asts.push_back(createAST(source, log()));
  asts.push_back(makeCmath(syms.builtins));

  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST(External_Func, Method) {
  const char *source = R"(
    import cmath;
  
    func min(x: real, y: real) {
      return x.isless(y) ? x : y;
    }
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  
  asts.push_back(createAST(source, log()));
  
  AST cmath;
  cmath.name = "cmath";
  auto isless = make_retain<ast::ExtFunc>();
  isless->name = "isless";
  isless->receiver = {ast::ParamRef::val, syms.builtins.Real};
  isless->params.push_back({ast::ParamRef::val, syms.builtins.Real});
  isless->ret = syms.builtins.Bool;
  cmath.global.push_back(std::move(isless));
  asts.push_back(std::move(cmath));
  
  const ModuleOrder order = findModuleOrder(asts, log());
  compileModules(syms, order, asts, log());
}

TEST(External_Func, Address) {
  const char *source = R"(
    import cmath;
  
    let ptr = sqrt;
  )";
  
  Symbols syms = initModules(log());
  ASTs asts;
  
  asts.push_back(createAST(source, log()));
  asts.push_back(makeCmath(syms.builtins));
  
  const ModuleOrder order = findModuleOrder(asts, log());
  EXPECT_THROW(compileModules(syms, order, asts, log()), FatalError);
}

TEST(External_Func, Inside_func) {
  Symbols syms = initModules(log());
  auto func = make_retain<ast::Func>();
  func->name = "func";
  func->body.nodes.push_back(makeSqrt(syms.builtins));
  AST cmath;
  cmath.global.push_back(std::move(func));
  EXPECT_THROW(compileModule(syms, cmath, log()), FatalError);
}

#undef EXPECT_SUCCEEDS
#undef EXPECT_FAILS

}
