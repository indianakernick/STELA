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
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Func - Redef with params, {
    const char *source = R"(
      func myFunction(i: sint, f: real) {
        
      }
      func myFunction(i: sint, f: real) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Func - Redef weak alias, {
    const char *source = R"(
      type Number = sint;
      func myFunction(i: Number) {
        
      }
      func myFunction(i: sint) {
        
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
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
    ASSERT_THROWS(createSym(source, log), FatalError);
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
    ASSERT_THROWS(createSym(source, log), FatalError);
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
    createSym(source, log);
  });

  TEST(Func - Overloading, {
    const char *source = R"(
      func myFunction(n: real) {
        
      }
      func myFunction(n: sint) {
        
      }
    )";
    createSym(source, log);
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
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Sym - Undefined, {
    const char *source = R"(
      func myFunction(i: Number) {
        
      }
      func myFunction(i: sint) {
        
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
  
  TEST(Sym - Type in func, {
    const char *source = R"(
      func fn() {
        type Number = real;
        let n: Number = 4.3;
      }
    )";
    createSym(source, log);
  });
  
  TEST(Var - Var type inference, {
    const char *source = R"(
      var integer = 5;
      var alsoInteger: sint = 5;
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
      let float = 3.14;
      let alsoFloat: real = 3.14;
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
      let big: uint = 4294967295;
      let neg: sint = -2147483648;
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
      let num: sint = 0;
      //var f: (real, inout [char]) -> [real];
      //var m: [{real: sint}];
      //let a: [real] = [1.2, 3.4, 5.6];
    )";
    createSym(source, log);
  });
  
  TEST(Var - Redefine var, {
    const char *source = R"(
      var x: sint;
      var x: real;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Var - Redefine let, {
    const char *source = R"(
      let x: sint = 0;
      let x: real = 0.0;
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Var - Type mismatch, {
    const char *source = R"(
      let x: sint = 2.5;
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
    createSym(source, log);
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
        fn: sint;
      };
    
      func (self: MyStruct) fn() {}
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Struct - Dup member, {
    const char *source = R"(
      type MyStruct struct {
        m: sint;
        m: real;
      };
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
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
    ASSERT_THROWS(createSym(source, log), FatalError);
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
        let test3 = ~(5u);
        let test4 = !(false);
      }
    )";
    createSym(source, log);
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
  
  TEST(Expr - Storing void, {
    const char *source = R"(
      func nothing() {}
    
      func main() {
        let void = nothing();
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expr - Access field on void, {
    const char *source = R"(
      func nothing() {}
    
      func main() {
        let value = nothing().field;
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
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
    ASSERT_THROWS(createSym(source, log), FatalError);
  });
  
  TEST(Expr - Call a float, {
    const char *source = R"(
      func main() {
        let fl = 3.0;
        fl(5.0);
      }
    )";
    ASSERT_THROWS(createSym(source, log), FatalError);
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
    createSym(source, log);
  });
});
