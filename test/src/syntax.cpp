//
//  syntax.cpp
//  Test
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include <gtest/gtest.h>
#include <STELA/syntax analysis.hpp>

namespace {

using namespace stela;
using namespace stela::ast;

#define ASSERT_DOWN_CAST(NAME, TYPE, EXPR)                                      \
  ASSERT_TRUE(EXPR);                                                            \
  auto *NAME = dynamic_cast<const TYPE *>((EXPR).get());                        \
  ASSERT_TRUE(NAME);

#define IS_BOP(NAME, EXPR, OPERATOR)                                            \
  ASSERT_DOWN_CAST(NAME, BinaryExpr, EXPR);                                     \
  EXPECT_EQ(NAME->oper, BinOp::OPERATOR);                                       \

#define IS_AOP(NAME, EXPR, OPERATOR)                                            \
  ASSERT_DOWN_CAST(NAME, CompAssign, EXPR);                                     \
  EXPECT_EQ(NAME->oper, AssignOp::OPERATOR);                                    \

#define IS_UOP(NAME, EXPR, OPERATOR)                                            \
  ASSERT_DOWN_CAST(NAME, UnaryExpr, EXPR);                                      \
  EXPECT_EQ(NAME->oper, UnOp::OPERATOR);                                        \

#define IS_NUM(EXPR, NUMBER)                                                    \
  {                                                                             \
    ASSERT_DOWN_CAST(num, NumberLiteral, EXPR);                                 \
    EXPECT_EQ(num->literal, NUMBER);                                            \
  } do{}while(0)

#define IS_ID(EXPR, IDENTIFIER)                                                 \
  {                                                                             \
    ASSERT_DOWN_CAST(id, Identifier, EXPR);                                     \
    EXPECT_EQ(id->name, IDENTIFIER);                                            \
  }

LogSink &log() {
  static StreamSink stream;
  static FilterSink filter{stream, LogPri::info};
  return filter;
}

TEST(Basic, No_tokens) {
  const AST ast = createAST(Tokens{}, log());
  EXPECT_TRUE(ast.global.empty());
  EXPECT_EQ(ast.name, "main");
  EXPECT_TRUE(ast.imports.empty());
}

TEST(Basic, Empty_source) {
  const AST ast = createAST("", log());
  EXPECT_TRUE(ast.global.empty());
  EXPECT_EQ(ast.name, "main");
  EXPECT_TRUE(ast.imports.empty());
}

TEST(Basic, End_of_input) {
  EXPECT_THROW(createAST("type", log()), FatalError);
}

TEST(Basic, Silent_logger) {
  std::cerr << "Testing silent logger\n";
  const char *source = R"(
    ;;;
  
    if (true) {}
  )";
  try {
    NullSink noLog;
    createAST(source, noLog);
  } catch (FatalError &e) {
    std::cerr << "Should have got nothing but this " << e.what() << " exception\n";
  }
}

TEST(Modules, Simple) {
  const char *source = R"(
    module MyModule;
  
    func yup() {}
  
    import OtherModule;
    import CoolModule;
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  EXPECT_EQ(ast.name, "MyModule");
  EXPECT_EQ(ast.imports.size(), 2);
  EXPECT_EQ(ast.imports[0], "OtherModule");
  EXPECT_EQ(ast.imports[1], "CoolModule");
}

TEST(Modules, String_name) {
  const char *source = R"(
    module "MyModule";
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Func, Empty) {
  const char *source = R"(
    func empty() {}
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  EXPECT_EQ(func->name, "empty");
  EXPECT_TRUE(func->params.empty());
}

TEST(Func, One_param) {
  const char *source = R"(
    func oneParam(one: Int) -> [Float] {}
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  EXPECT_EQ(func->name, "oneParam");
  EXPECT_EQ(func->params.size(), 1);
  
  EXPECT_EQ(func->params[0].name, "one");
  EXPECT_EQ(func->params[0].ref, ParamRef::val);
  ASSERT_DOWN_CAST(intType, NamedType, func->params[0].type);
  EXPECT_EQ(intType->name, "Int");
  
  ASSERT_DOWN_CAST(array, ArrayType, func->ret);
  ASSERT_DOWN_CAST(type, NamedType, array->elem);
  EXPECT_EQ(type->name, "Float");
}

TEST(Func, Two_param) {
  const char *source = R"(
    func swap(first: ref Int, second: ref Int) {}
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  EXPECT_EQ(func->name, "swap");
  EXPECT_EQ(func->params.size(), 2);
  
  EXPECT_EQ(func->params[0].name, "first");
  EXPECT_EQ(func->params[1].name, "second");
  EXPECT_EQ(func->params[0].ref, ParamRef::ref);
  EXPECT_EQ(func->params[1].ref, ParamRef::ref);
}

TEST(Func, Two_param_no_comma) {
  const char *source = R"(
    func woops(first: String oh_no: Int) {}
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Type, Keyword) {
  const char *source = R"(
    type dummy = func;
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Type, Name) {
  const char *source = R"(
    type dummy = name;
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(alias, TypeAlias, ast.global[0]);
  EXPECT_EQ(alias->name, "dummy");
  
  ASSERT_DOWN_CAST(named, NamedType, alias->type);
  EXPECT_EQ(named->name, "name");
}

TEST(Type, Array_of_functions) {
  const char *source = R"(
    type dummy = [func(ref Int, ref Double) -> Char];
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(alias, TypeAlias, ast.global[0]);
  EXPECT_EQ(alias->name, "dummy");
  
  ASSERT_DOWN_CAST(array, ArrayType, alias->type);
  ASSERT_DOWN_CAST(func, FuncType, array->elem);
  EXPECT_EQ(func->params.size(), 2);
  EXPECT_EQ(func->params[0].ref, ParamRef::ref);
  EXPECT_EQ(func->params[1].ref, ParamRef::ref);
  
  ASSERT_DOWN_CAST(first, NamedType, func->params[0].type);
  EXPECT_EQ(first->name, "Int");
  ASSERT_DOWN_CAST(second, NamedType, func->params[1].type);
  EXPECT_EQ(second->name, "Double");
  ASSERT_DOWN_CAST(ret, NamedType, func->ret);
  EXPECT_EQ(ret->name, "Char");
}

TEST(Type, Function_no_ret_type) {
  const char *source = R"(
    type dummy = func(Int, Char);
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(alias, TypeAlias, ast.global[0]);
  EXPECT_EQ(alias->name, "dummy");
  EXPECT_FALSE(alias->strong);
  
  ASSERT_DOWN_CAST(func, FuncType, alias->type);
  EXPECT_EQ(func->params.size(), 2);
  EXPECT_FALSE(func->ret);
}

TEST(Type, Invalid) {
  const char *source = R"(
    type dummy = {Int};
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Stat, Block) {
  const char *source = R"(
    func dummy() {
      var yeah = expr;
      {
        let blocks_are_statements = expr;
        let just_like_C = expr;
      }
      let cool = expr;
      {}
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 4);
}

TEST(Stat, If) {
  const char *source = R"(
    func dummy() {
      if expr {
        var dummy0 = expr;
      }
      
      if (expr) {
        var dummy0 = expr;
        var dummy1 = expr;
      } else {
        var dummy0 = expr;
        var dummy1 = expr;
        var dummy2 = expr;
      }
      
      if (expr) {
        var dummy0 = expr;
        var dummy1 = expr;
        var dummy2 = expr;
        var dummy3 = expr;
      } else if (expr) {
        var dummy0 = expr;
        var dummy1 = expr;
        var dummy2 = expr;
        var dummy3 = expr;
        var dummy4 = expr;
      } else {
        var dummy0 = expr;
        var dummy1 = expr;
        var dummy2 = expr;
        var dummy3 = expr;
        var dummy4 = expr;
        var dummy5 = expr;
      }
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &fnBlock = func->body.nodes;
  EXPECT_EQ(fnBlock.size(), 3);
  
  {
    ASSERT_DOWN_CAST(ifNode, If, fnBlock[0]);
    EXPECT_TRUE(ifNode->cond);
    ASSERT_DOWN_CAST(block, Block, ifNode->body);
    EXPECT_EQ(block->nodes.size(), 1);
    EXPECT_FALSE(ifNode->elseBody);
  }
  
  {
    ASSERT_DOWN_CAST(ifNode, If, fnBlock[1]);
    EXPECT_TRUE(ifNode->cond);
    ASSERT_DOWN_CAST(block, Block, ifNode->body);
    EXPECT_EQ(block->nodes.size(), 2);
    ASSERT_DOWN_CAST(elseBlock, Block, ifNode->elseBody);
    EXPECT_EQ(elseBlock->nodes.size(), 3);
  }
  
  {
    ASSERT_DOWN_CAST(ifNode, If, fnBlock[2]);
    EXPECT_TRUE(ifNode->cond);
    ASSERT_DOWN_CAST(block, Block, ifNode->body);
    EXPECT_EQ(block->nodes.size(), 4);
    ASSERT_DOWN_CAST(elseIf, If, ifNode->elseBody);
    EXPECT_TRUE(elseIf->cond);
    ASSERT_DOWN_CAST(elseIfBlock, Block, elseIf->body);
    EXPECT_EQ(elseIfBlock->nodes.size(), 5);
    ASSERT_DOWN_CAST(elseIfElseBlock, Block, elseIf->elseBody);
    EXPECT_EQ(elseIfElseBlock->nodes.size(), 6);
  }
}

TEST(Stat, Keywords) {
  const char *source = R"(
    func dummy() {
      break;
      continue;
      return;
      return expr;
    }
  )";
  
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 4);
  
  ASSERT_DOWN_CAST(brake, Break, block[0]);
  ASSERT_DOWN_CAST(continu, Continue, block[1]);
  
  {
    ASSERT_DOWN_CAST(retVoid, Return, block[2]);
    EXPECT_FALSE(retVoid->expr);
  }
  
  {
    ASSERT_DOWN_CAST(retExpr, Return, block[3]);
    EXPECT_TRUE(retExpr->expr);
  }
}

TEST(Stat, Loops) {
  const char *source = R"(
    func dummy() {
      while (expr) {}
      while expr {}
      for (i := expr; expr; i++) {}
      for (; expr; i++) {}
      for (; expr; ) {}
    }
  )";
  
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 5);
  
  {
    ASSERT_DOWN_CAST(whileNode, While, block[0]);
    EXPECT_TRUE(whileNode->cond);
    ASSERT_DOWN_CAST(body, Block, whileNode->body);
  }
  {
    ASSERT_DOWN_CAST(whileNode, While, block[1]);
    EXPECT_TRUE(whileNode->cond);
    ASSERT_DOWN_CAST(body, Block, whileNode->body);
  }
  {
    ASSERT_DOWN_CAST(forNode, For, block[2]);
    ASSERT_DOWN_CAST(init, DeclAssign, forNode->init);
    EXPECT_TRUE(forNode->cond);
    ASSERT_DOWN_CAST(incr, IncrDecr, forNode->incr);
    ASSERT_DOWN_CAST(body, Block, forNode->body);
  }
  {
    ASSERT_DOWN_CAST(forNode, For, block[3]);
    EXPECT_FALSE(forNode->init);
    EXPECT_TRUE(forNode->cond);
    ASSERT_DOWN_CAST(incr, IncrDecr, forNode->incr);
    ASSERT_DOWN_CAST(body, Block, forNode->body);
  }
  {
    ASSERT_DOWN_CAST(forNode, For, block[4]);
    EXPECT_FALSE(forNode->init);
    EXPECT_TRUE(forNode->cond);
    EXPECT_FALSE(forNode->incr);
    ASSERT_DOWN_CAST(body, Block, forNode->body);
  }
}

TEST(Switch, Standard) {
  const char *source = R"(
    func dummy() {
      switch (expr) {}
      
      switch expr {
        case expr {
          expr++;
          continue;
        }
        case expr {
          expr++;
          break;
        }
        case (expr) {
          expr++;
          return expr;
        }
        default {
          expr++;
          return;
        }
      }
    }
  )";
  
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &fnBlock = func->body.nodes;
  EXPECT_EQ(fnBlock.size(), 2);
  
  {
    ASSERT_DOWN_CAST(switchNode, Switch, fnBlock[0]);
    EXPECT_TRUE(switchNode->expr);
    EXPECT_TRUE(switchNode->cases.empty());
  }
  {
    ASSERT_DOWN_CAST(switchNode, Switch, fnBlock[1]);
    EXPECT_TRUE(switchNode->expr);
    const auto &cases = switchNode->cases;
    EXPECT_EQ(cases.size(), 4);
    {
      EXPECT_TRUE(cases[0].expr);
      ASSERT_DOWN_CAST(block, Block, cases[0].body);
      EXPECT_TRUE(block->nodes[0]);
      ASSERT_DOWN_CAST(continu, Continue, block->nodes[1]);
    }
    {
      EXPECT_TRUE(cases[1].expr);
      ASSERT_DOWN_CAST(block, Block, cases[1].body);
      EXPECT_TRUE(block->nodes[0]);
      ASSERT_DOWN_CAST(brake, Break, block->nodes[1]);
    }
    {
      EXPECT_TRUE(cases[2].expr);
      ASSERT_DOWN_CAST(block, Block, cases[2].body);
      EXPECT_TRUE(block->nodes[0]);
      ASSERT_DOWN_CAST(ret, Return, block->nodes[1]);
      EXPECT_TRUE(ret->expr);
    }
    {
      EXPECT_FALSE(cases[3].expr);
      ASSERT_DOWN_CAST(block, Block, cases[3].body);
      EXPECT_TRUE(block->nodes[0]);
      ASSERT_DOWN_CAST(ret, Return, block->nodes[1]);
      EXPECT_FALSE(ret->expr);
    }
  }
}

TEST(Switch, No_statement) {
  const char *source = R"(
    func dummy() {
      switch (expr) {
        case (expr)
          |; // wait, that's not a statement
      }
    }
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Switch, No_case) {
  const char *source = R"(
    func dummy() {
      switch (expr) {
        break;
      }
    }
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Decl, Var_and_let) {
  const char *source = R"(
    func dummy() {
      var noInit: Double;
      var initAndType: Int = expr;
      var deducedVar = expr;
      let myLet: MyClass = expr;
      let deducedLet = expr;
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 5);
  
  {
    ASSERT_DOWN_CAST(noInit, Var, block[0]);
    EXPECT_EQ(noInit->name, "noInit");
    EXPECT_FALSE(noInit->expr);
    ASSERT_DOWN_CAST(noInitType, NamedType, noInit->type);
    EXPECT_EQ(noInitType->name, "Double");
  }
  
  {
    ASSERT_DOWN_CAST(initAndType, Var, block[1]);
    EXPECT_EQ(initAndType->name, "initAndType");
    EXPECT_TRUE(initAndType->expr);
    ASSERT_DOWN_CAST(initAndTypeT, NamedType, initAndType->type);
    EXPECT_EQ(initAndTypeT->name, "Int");
  }
  
  {
    ASSERT_DOWN_CAST(deducedVar, Var, block[2]);
    EXPECT_EQ(deducedVar->name, "deducedVar");
    EXPECT_TRUE(deducedVar->expr);
    EXPECT_FALSE(deducedVar->type);
  }
  
  {
    ASSERT_DOWN_CAST(myLet, Let, block[3]);
    EXPECT_EQ(myLet->name, "myLet");
    EXPECT_TRUE(myLet->expr);
    ASSERT_DOWN_CAST(myLetType, NamedType, myLet->type);
    EXPECT_EQ(myLetType->name, "MyClass");
  }
  
  {
    ASSERT_DOWN_CAST(deducedLet, Let, block[4]);
    EXPECT_EQ(deducedLet->name, "deducedLet");
    EXPECT_TRUE(deducedLet->expr);
    EXPECT_FALSE(deducedLet->type);
  }
}

TEST(Decl, Extern) {
  const char *source = R"(
    func a() {}
    var b = 0;
    let c = 0;
  
    extern func d() {}
    extern var e = 0;
    extern let f = 0;
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 6);
  
  ASSERT_DOWN_CAST(a, Func, ast.global[0]);
  EXPECT_EQ(a->name, "a");
  EXPECT_FALSE(a->external);
  
  ASSERT_DOWN_CAST(b, Var, ast.global[1]);
  EXPECT_EQ(b->name, "b");
  EXPECT_FALSE(b->external);
  
  ASSERT_DOWN_CAST(c, Let, ast.global[2]);
  EXPECT_EQ(c->name, "c");
  EXPECT_FALSE(c->external);
  
  ASSERT_DOWN_CAST(d, Func, ast.global[3]);
  EXPECT_EQ(d->name, "d");
  EXPECT_TRUE(d->external);
  
  ASSERT_DOWN_CAST(e, Var, ast.global[4]);
  EXPECT_EQ(e->name, "e");
  EXPECT_TRUE(e->external);
  
  ASSERT_DOWN_CAST(f, Let, ast.global[5]);
  EXPECT_EQ(f->name, "f");
  EXPECT_TRUE(f->external);
}

TEST(Decl, Extern_type) {
  const char *source = R"(
    extern type sint;
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Decl, Extern_local) {
  const char *source = R"(
    func fn() {
      extern var num = 0;
    }
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Stat, Global) {
  const char *source = R"(
    module my_awesome_module;
  
    ; // an extra semicolon never hurt anyone
  
    if (expr) {} // that's not a declaration
  )";
  // good test for the color logger because all priorities are used
  // can't use color logger for everything because Xcode Console doesn't
  // support color. ;-(
  ColorSink colorLog;
  EXPECT_THROW(createAST(source, colorLog), FatalError);
}

TEST(Struct, Empty) {
  const char *source = R"(
    type NoMembers struct {};
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(alias, TypeAlias, ast.global[0]);
  EXPECT_EQ(alias->name, "NoMembers");
  EXPECT_TRUE(alias->strong);
  ASSERT_DOWN_CAST(strut, StructType, alias->type);
  EXPECT_TRUE(strut->fields.empty());
}

TEST(Struct, Vars) {
  const char *source = R"(
    type Vec2 struct {
      x: Float;
      y: Float;
    };
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(alias, TypeAlias, ast.global[0]);
  EXPECT_EQ(alias->name, "Vec2");
  ASSERT_DOWN_CAST(strut, StructType, alias->type);
  EXPECT_EQ(strut->fields.size(), 2);
  
  {
    const Field &field = strut->fields[0];
    EXPECT_EQ(field.name, "x");
  }
  {
    const Field &field = strut->fields[1];
    EXPECT_EQ(field.name, "y");
  }
}

TEST(Struct, Functions) {
  const char *source = R"(
    type Vec2 struct {
      x: Float;
      y: Float;
    };
  
    func (self: ref Vec2) add(other: Vec2) {
      self.x += other.x;
      self.y += other.y;
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 2);
  ASSERT_DOWN_CAST(alias, TypeAlias, ast.global[0]);
  EXPECT_EQ(alias->name, "Vec2");
  ASSERT_DOWN_CAST(strut, StructType, alias->type);
  EXPECT_EQ(strut->fields.size(), 2);
  
  {
    const Field &field = strut->fields[0];
    EXPECT_EQ(field.name, "x");
  }
  {
    const Field &field = strut->fields[1];
    EXPECT_EQ(field.name, "y");
  }
  
  ASSERT_DOWN_CAST(func, Func, ast.global[1]);
  EXPECT_EQ(func->name, "add");
  EXPECT_TRUE(func->receiver);
  EXPECT_EQ(func->params.size(), 1);
  EXPECT_FALSE(func->ret);
}

TEST(Struct, Bad_member) {
  const char *source = R"(
    struct C {
      int x;
    };
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Expr, Simple_literals) {
  const char *source = R"(
    func dummy() {
      var a = "This is a string";
      var a = 'c';
      var a = 73;
      var a = true;
      var a = false;
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 5);
  
  {
    ASSERT_DOWN_CAST(var, Var, block[0]);
    ASSERT_DOWN_CAST(lit, StringLiteral, var->expr);
    EXPECT_EQ(lit->literal, "This is a string");
  }
  {
    ASSERT_DOWN_CAST(var, Var, block[1]);
    ASSERT_DOWN_CAST(lit, CharLiteral, var->expr);
    EXPECT_EQ(lit->literal, "c");
  }
  {
    ASSERT_DOWN_CAST(var, Var, block[2]);
    ASSERT_DOWN_CAST(lit, NumberLiteral, var->expr);
    EXPECT_EQ(lit->literal, "73");
  }
  {
    ASSERT_DOWN_CAST(var, Var, block[3]);
    ASSERT_DOWN_CAST(lit, BoolLiteral, var->expr);
    EXPECT_EQ(lit->value, true);
  }
  {
    ASSERT_DOWN_CAST(var, Var, block[4]);
    ASSERT_DOWN_CAST(lit, BoolLiteral, var->expr);
    EXPECT_EQ(lit->value, false);
  }
}

TEST(Expr, Array) {
  const char *source = R"(
    func dummy() {
      var empty = [];
      var seven = [7];
      var seven_ate_nine = [7, 8, 9];
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 3);
  
  {
    ASSERT_DOWN_CAST(var, Var, block[0]);
    ASSERT_DOWN_CAST(lit, ArrayLiteral, var->expr);
    EXPECT_TRUE(lit->exprs.empty());
  }
  {
    ASSERT_DOWN_CAST(var, Var, block[1]);
    ASSERT_DOWN_CAST(lit, ArrayLiteral, var->expr);
    EXPECT_EQ(lit->exprs.size(), 1);
    ASSERT_DOWN_CAST(seven, NumberLiteral, lit->exprs[0]);
    EXPECT_EQ(seven->literal, "7");
  }
  {
    ASSERT_DOWN_CAST(var, Var, block[2]);
    ASSERT_DOWN_CAST(lit, ArrayLiteral, var->expr);
    EXPECT_EQ(lit->exprs.size(), 3);
    ASSERT_DOWN_CAST(seven, NumberLiteral, lit->exprs[0]);
    EXPECT_EQ(seven->literal, "7");
    ASSERT_DOWN_CAST(eight, NumberLiteral, lit->exprs[1]);
    EXPECT_EQ(eight->literal, "8");
    ASSERT_DOWN_CAST(nine, NumberLiteral, lit->exprs[2]);
    EXPECT_EQ(nine->literal, "9");
  }
}

TEST(Expr, Init_list) {
  const char *source = R"(
    let vec: Vec2 = {2.0, 3.0};
    let zero: sint = {};
    let abc = make Tup3 {"a", "b", "c"};
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 3);
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[0]);
    EXPECT_EQ(let->name, "vec");
      ASSERT_DOWN_CAST(type, NamedType, let->type);
        EXPECT_EQ(type->name, "Vec2");
      ASSERT_DOWN_CAST(list, InitList, let->expr);
        EXPECT_EQ(list->exprs.size(), 2);
        IS_NUM(list->exprs[0], "2.0");
        IS_NUM(list->exprs[1], "3.0");
  }
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[1]);
    EXPECT_EQ(let->name, "zero");
      ASSERT_DOWN_CAST(type, NamedType, let->type);
        EXPECT_EQ(type->name, "sint");
      ASSERT_DOWN_CAST(list, InitList, let->expr);
        EXPECT_TRUE(list->exprs.empty());
  }
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[2]);
    EXPECT_EQ(let->name, "abc");
      EXPECT_FALSE(let->type);
      ASSERT_DOWN_CAST(make, Make, let->expr);
        ASSERT_DOWN_CAST(type, NamedType, make->type);
          EXPECT_EQ(type->name, "Tup3");
        ASSERT_DOWN_CAST(list, InitList, make->expr);
          EXPECT_EQ(list->exprs.size(), 3);
          ASSERT_DOWN_CAST(a, StringLiteral, list->exprs[0]);
            EXPECT_EQ(a->literal, "a");
          ASSERT_DOWN_CAST(b, StringLiteral, list->exprs[1]);
            EXPECT_EQ(b->literal, "b");
          ASSERT_DOWN_CAST(c, StringLiteral, list->exprs[2]);
            EXPECT_EQ(c->literal, "c");
  }
}

TEST(Expr, Lambda) {
  const char *source = R"(
    let nothing = func() {};
    let identity = func(n: Int) -> Int {
      return expr;
    };
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 2);
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[0]);
    ASSERT_DOWN_CAST(lambda, Lambda, let->expr);
    EXPECT_TRUE(lambda->params.empty());
    EXPECT_FALSE(lambda->ret);
    EXPECT_TRUE(lambda->body.nodes.empty());
  }
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[1]);
    ASSERT_DOWN_CAST(lambda, Lambda, let->expr);
    EXPECT_EQ(lambda->params.size(), 1);
    
    EXPECT_EQ(lambda->params[0].name, "n");
    ASSERT_DOWN_CAST(n, NamedType, lambda->params[0].type);
    EXPECT_EQ(n->name, "Int");
    
    EXPECT_EQ(lambda->body.nodes.size(), 1);
    ASSERT_DOWN_CAST(returnStat, Return, lambda->body.nodes[0]);
    EXPECT_TRUE(returnStat->expr);
  }
}

TEST(Expr, Identifier) {
  const char *source = R"(
    let five = 5;
    let alsoFive = five;
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 2);
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[0]);
    EXPECT_EQ(let->name, "five");
    ASSERT_DOWN_CAST(num, NumberLiteral, let->expr);
    EXPECT_EQ(num->literal, "5");
  }
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[1]);
    EXPECT_EQ(let->name, "alsoFive");
    ASSERT_DOWN_CAST(ident, Identifier, let->expr);
    EXPECT_EQ(ident->name, "five");
  }
}

TEST(Expr, Make) {
  const char *source = R"(
    let five = make uint make sint 5.0;
    let six = make real (6);
    let one = make sint !false;
    let two = make real 6 / 3.0;
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 4);
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[0]);
    EXPECT_EQ(let->name, "five");
    ASSERT_DOWN_CAST(make_uint, Make, let->expr);
      ASSERT_DOWN_CAST(uint, NamedType, make_uint->type);
        EXPECT_EQ(uint->name, "uint");
      ASSERT_DOWN_CAST(make_sint, Make, make_uint->expr);
        ASSERT_DOWN_CAST(sint, NamedType, make_sint->type);
          EXPECT_EQ(sint->name, "sint");
        IS_NUM(make_sint->expr, "5.0");
  }
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[1]);
    EXPECT_EQ(let->name, "six");
    ASSERT_DOWN_CAST(make_real, Make, let->expr);
      ASSERT_DOWN_CAST(real, NamedType, make_real->type);
        EXPECT_EQ(real->name, "real");
      IS_NUM(make_real->expr, "6");
  }
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[2]);
    EXPECT_EQ(let->name, "one");
    ASSERT_DOWN_CAST(make_sint, Make, let->expr);
      ASSERT_DOWN_CAST(sint, NamedType, make_sint->type);
        EXPECT_EQ(sint->name, "sint");
      IS_UOP(bnot, make_sint->expr, bool_not);
        ASSERT_DOWN_CAST(boolLit, BoolLiteral, bnot->expr);
          EXPECT_FALSE(boolLit->value);
  }
  
  {
    ASSERT_DOWN_CAST(let, Let, ast.global[3]);
    EXPECT_EQ(let->name, "two");
    IS_BOP(div, let->expr, div);
      ASSERT_DOWN_CAST(make_real, Make, div->left);
        ASSERT_DOWN_CAST(real, NamedType, make_real->type);
          EXPECT_EQ(real->name, "real");
        IS_NUM(make_real->expr, "6");
      IS_NUM(div->right, "3.0");
  }
}

TEST(Expr, Math) {
  /*
  
    add
   /   \
  3    div
     /     \
  mul       sub
 /   \     /   \
4     2   1     mod
               /   \
              5     6
   
  */

  const char *source = "let a = 3 + 4 * 2 / (1 - 5 % 6);";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(let, Let, ast.global[0]);
    IS_BOP(add, let->expr, add);
      IS_NUM(add->left, "3");
      IS_BOP(div, add->right, div);
        IS_BOP(mul, div->left, mul);
          IS_NUM(mul->left, "4");
          IS_NUM(mul->right, "2");
        IS_BOP(sub, div->right, sub);
          IS_NUM(sub->left, "1");
          IS_BOP(mod, sub->right, mod);
            IS_NUM(mod->left, "5");
            IS_NUM(mod->right, "6");
}

TEST(Expr, Assign_and_compare) {
  /*
  
    assign_add
   /          \
yeah        bool_and
           /        \
         eq          lt
        /  \        /  \
       1    two  four   5
  
  */
  
  const char *source = R"(
    func dummy() {
      yeah += 1 == two && four < 5;
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 1);
  
  IS_AOP(assign_add, block[0], add);
    IS_ID(assign_add->left, "yeah");
    IS_BOP(bool_and, assign_add->right, bool_and);
      IS_BOP(eq, bool_and->left, eq);
        IS_NUM(eq->left, "1");
        IS_ID(eq->right, "two");
      IS_BOP(lt, bool_and->right, lt);
        IS_ID(lt->left, "four");
        IS_NUM(lt->right, "5");
}

TEST(Expr, Unary) {
  /*
  
     assign_sub
    /          \
   n           sub
              /   \
            neg   neg
             |     |
             n    add
                 /   \
          bit_not     bool_not
              |         |
              5         0
  
  */

  const char *source = R"(
    func dummy() {
      n++;
      n--;
      n -= -n - -(~5 + !0);
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 3);
  
  ASSERT_DOWN_CAST(incr, IncrDecr, block[0]);
  EXPECT_TRUE(incr->incr);
  ASSERT_DOWN_CAST(decr, IncrDecr, block[1]);
  EXPECT_FALSE(decr->incr);
  IS_AOP(assign, block[2], sub);
    IS_ID(assign->left, "n");
    IS_BOP(sub, assign->right, sub);
      IS_UOP(lneg, sub->left, neg);
        IS_ID(lneg->expr, "n");
      IS_UOP(rneg, sub->right, neg);
        IS_BOP(add, rneg->expr, add);
          IS_UOP(bitNot, add->left, bit_not);
            IS_NUM(bitNot->expr, "5");
          IS_UOP(boolNot, add->right, bool_not);
            IS_NUM(boolNot->expr, "0");
}

TEST(Expr, Ternary) {
  /*
  
   assign_mul
  /          \
 e         ternary
          /   |   \
        le    a    div
       /  \       /   \
      a    d     a     d
   
  */

  const char *source = R"(
    func dummy() {
      e *= a <= d ? a : a / d;
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 1);
  
  IS_AOP(assignMul, block[0], mul);
    IS_ID(assignMul->left, "e");
    ASSERT_DOWN_CAST(tern, Ternary, assignMul->right);
      IS_BOP(le, tern->cond, le);
        IS_ID(le->left, "a");
        IS_ID(le->right, "d");
      IS_ID(tern->troo, "a");
      IS_BOP(div, tern->fols, div);
        IS_ID(div->left, "a");
        IS_ID(div->right, "d");
}

TEST(Expr, Member_and_subscript) {
  /*
                     subscript1
                    /          \
          subscript2            h
         /          \
     mem1            subscript3
    /    \          /          \
   mem2   c     mem3            g
  /    \       /    \
 a      b     e      f
  
  */
  
  const char *source = R"(
    func dummy() {
      let test = a.b.c[e.f[g]][h];
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 1);
  
  ASSERT_DOWN_CAST(let, Let, block[0]);
  ASSERT_DOWN_CAST(sub1, Subscript, let->expr);
    ASSERT_DOWN_CAST(sub2, Subscript, sub1->object);
      ASSERT_DOWN_CAST(mem1, MemberIdent, sub2->object);
        ASSERT_DOWN_CAST(mem2, MemberIdent, mem1->object);
          IS_ID(mem2->object, "a");
          EXPECT_EQ(mem2->member, "b");
        EXPECT_EQ(mem1->member, "c");
      ASSERT_DOWN_CAST(sub3, Subscript, sub2->index);
        ASSERT_DOWN_CAST(mem3, MemberIdent, sub3->object);
          IS_ID(mem3->object, "e");
          EXPECT_EQ(mem3->member, "f");
        IS_ID(sub3->index, "g");
    IS_ID(sub1->index, "h");
}

TEST(Expr, Bits) {
  /*
  
             bool_or
            /       \
          /           \
        ne             bit_or
       /  \           /      \
    not   shr      and        xor
   /     /   \    /   \      /   \
  a   shl     3  c     d    e     f
     /   \
    b     2
  
  */

  const char *source = R"(
    func dummy() {
      let test = ~a != b << 2 >> 3 || c & d | e ^ f;
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 1);
  ASSERT_DOWN_CAST(let, Let, block[0]);
  
  IS_BOP(boolOr, let->expr, bool_or);
    IS_BOP(ne, boolOr->left, ne);
      IS_UOP(bnot, ne->left, bit_not);
        IS_ID(bnot->expr, "a");
      IS_BOP(shr, ne->right, bit_shr);
        IS_BOP(shl, shr->left, bit_shl);
          IS_ID(shl->left, "b");
          IS_NUM(shl->right, "2");
        IS_NUM(shr->right, "3");
    IS_BOP(bor, boolOr->right, bit_or);
      IS_BOP(band, bor->left, bit_and);
        IS_ID(band->left, "c");
        IS_ID(band->right, "d");
      IS_BOP(bxor, bor->right, bit_xor);
        IS_ID(bxor->left, "e");
        IS_ID(bxor->right, "f");
}

TEST(Expr, Function_call) {
  /*
  
                call1
               /     \
             /         \
         mem1           args
        /    \         /    \
     sub      b   call2      4
    /   \        /     \
   a     2   mem2       args
            /    \
       call3      d
      /     \
     c       args
              |
              3
  
  */
  
  const char *source = R"(
    func dummy() {
      a[2].b(c(3).d(), 4);
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 1);
  
  ASSERT_DOWN_CAST(assign, CallAssign, block[0]);
  auto *call1 = &assign->call;
    ASSERT_DOWN_CAST(mem1, MemberIdent, call1->func);
      ASSERT_DOWN_CAST(sub, Subscript, mem1->object);
        IS_ID(sub->object, "a");
        IS_NUM(sub->index, "2");
      EXPECT_EQ(mem1->member, "b");
    EXPECT_EQ(call1->args.size(), 2);
      ASSERT_DOWN_CAST(call2, FuncCall, call1->args[0]);
        ASSERT_DOWN_CAST(mem2, MemberIdent, call2->func);
          ASSERT_DOWN_CAST(call3, FuncCall, mem2->object);
            IS_ID(call3->func, "c");
            EXPECT_EQ(call3->args.size(), 1);
              IS_NUM(call3->args[0], "3");
          EXPECT_EQ(mem2->member, "d");
        EXPECT_TRUE(call2->args.empty());
      IS_NUM(call1->args[1], "4");
}

TEST(Expr, Assign) {
  /*
  
      or
     /  \
    a    ge
        /  \
      gt    d
     /  \
    b    c
  
  */
  const char *source = R"(
    func dummy() {
      a %= b;
      a <<= b;
      a >>= b;
      a &= b;
      a ^= b;
      a |= b > c >= d;
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 6);
  
  IS_AOP(mod, block[0], mod);
    IS_ID(mod->left, "a");
    IS_ID(mod->right, "b");
  IS_AOP(shl, block[1], bit_shl);
    IS_ID(shl->left, "a");
    IS_ID(shl->right, "b");
  IS_AOP(shr, block[2], bit_shr);
    IS_ID(shr->left, "a");
    IS_ID(shr->right, "b");
  IS_AOP(band, block[3], bit_and);
    IS_ID(band->left, "a");
    IS_ID(band->right, "b");
  IS_AOP(bxor, block[4], bit_xor);
    IS_ID(bxor->left, "a");
    IS_ID(bxor->right, "b");
  IS_AOP(bor, block[5], bit_or);
    IS_ID(bor->left, "a");
    IS_BOP(ge, bor->right, ge);
      IS_BOP(gt, ge->left, gt);
        IS_ID(gt->left, "b");
        IS_ID(gt->right, "c");
      IS_ID(ge->right, "d");
}

TEST(Expr, Result_unused) {
  const char *source = R"(
    func dummy() {
      a.b.c;
    }
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Expr, Unary_plus) {
  const char *source = R"(
    func dummy() {
      let a = +b;
    }
  )";
  EXPECT_THROW(createAST(source, log()), FatalError);
}

TEST(Expr, Factorial) {
  /*
  
        fac
       /   \
 params     body
    |          |
    0       return
 /  |  \       |
n  val Int  ternary
           /   |   \
         eq    1    mul
        /  \       /   \
       n    0     n     call
                          |
                         sub
                        /   \
                       n     1
  
  */
  const char *source = R"(
    func fac(n: sint) {
      return n == 0 ? 1 : n * fac(n - 1);
    }
  )";
  const AST ast = createAST(source, log());
  EXPECT_EQ(ast.global.size(), 1);
  ASSERT_DOWN_CAST(func, Func, ast.global[0]);
  EXPECT_EQ(func->name, "fac");
  EXPECT_EQ(func->params.size(), 1);
    EXPECT_EQ(func->params[0].name, "n");
    EXPECT_EQ(func->params[0].ref, ParamRef::val);
    ASSERT_DOWN_CAST(sint, NamedType, func->params[0].type);
      EXPECT_EQ(sint->name, "sint");
  EXPECT_FALSE(func->ret);
  const auto &block = func->body.nodes;
  EXPECT_EQ(block.size(), 1);
  ASSERT_DOWN_CAST(ret, Return, block[0]);
    ASSERT_DOWN_CAST(ternary, Ternary, ret->expr);
      IS_BOP(eq, ternary->cond, eq);
        IS_ID(eq->left, "n");
        IS_NUM(eq->right, "0");
      IS_NUM(ternary->troo, "1");
      IS_BOP(mul, ternary->fols, mul);
        IS_ID(mul->left, "n");
        ASSERT_DOWN_CAST(call, FuncCall, mul->right);
          IS_ID(call->func, "fac");
          EXPECT_EQ(call->args.size(), 1);
            IS_BOP(sub, call->args[0], sub);
              IS_ID(sub->left, "n");
              IS_NUM(sub->right, "1");
}

#undef IS_ID
#undef IS_NUM
#undef IS_UOP
#undef IS_AOP
#undef IS_BOP
#undef ASSERT_DOWN_CAST

}
