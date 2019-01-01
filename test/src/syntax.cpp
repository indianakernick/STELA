//
//  syntax.cpp
//  Test
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax.hpp"

#include "macros.hpp"
#include <STELA/syntax analysis.hpp>

using namespace stela;
using namespace stela::ast;

#define IS_BOP(EXP, OPERATOR)                                                   \
  [&] {                                                                         \
    auto *op = ASSERT_DOWN_CAST(const BinaryExpr, EXP);                         \
    ASSERT_EQ(op->oper, BinOp::OPERATOR);                                       \
    return op;                                                                  \
  }()

#define IS_AOP(EXP, OPERATOR)                                                   \
  [&] {                                                                         \
    auto *op = ASSERT_DOWN_CAST(const CompAssign, EXP);                         \
    ASSERT_EQ(op->oper, AssignOp::OPERATOR);                                    \
    return op;                                                                  \
  }()

#define IS_UOP(EXP, OPERATOR)                                                   \
  [&] {                                                                         \
    auto *op = ASSERT_DOWN_CAST(const UnaryExpr, EXP);                          \
    ASSERT_EQ(op->oper, UnOp::OPERATOR);                                        \
    return op;                                                                  \
  }()

#define IS_NUM(EXP, NUMBER)                                                     \
  {                                                                             \
    auto *num = ASSERT_DOWN_CAST(const NumberLiteral, EXP);                     \
    ASSERT_EQ(num->value, NUMBER);                                              \
  } do{}while(0)

#define IS_ID(EXP, IDENTIFIER)                                                  \
  {                                                                             \
    auto *id = ASSERT_DOWN_CAST(const Identifier, EXP);                         \
    ASSERT_EQ(id->name, IDENTIFIER);                                            \
  } do{}while(0)

TEST_GROUP(Syntax, {
  StreamSink stream;
  FilterSink log{stream, LogPri::info};

  TEST(No tokens, {
    const AST ast = createAST(Tokens{}, log);
    ASSERT_TRUE(ast.global.empty());
    ASSERT_EQ(ast.name, "main");
    ASSERT_TRUE(ast.imports.empty());
  });
  
  TEST(Empty source, {
    const AST ast = createAST("", log);
    ASSERT_TRUE(ast.global.empty());
    ASSERT_EQ(ast.name, "main");
    ASSERT_TRUE(ast.imports.empty());
  });
  
  TEST(Modules, {
    const char *source = R"(
      module MyModule;
    
      func yup() {}
    
      import OtherModule;
      import CoolModule;
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    ASSERT_EQ(ast.name, "MyModule");
    ASSERT_EQ(ast.imports.size(), 2);
    ASSERT_EQ(ast.imports[0], "OtherModule");
    ASSERT_EQ(ast.imports[1], "CoolModule");
  });
  
  TEST(Module string name, {
    const char *source = R"(
      module "MyModule";
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(EOF, {
    ASSERT_THROWS(createAST("type", log), FatalError);
  });
  
  TEST(Silent Logger, {
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
  });
  
  TEST(Func - empty, {
    const char *source = R"(
      func empty() {}
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    ASSERT_EQ(func->name, "empty");
    ASSERT_TRUE(func->params.empty());
  });
  
  TEST(Func - one param, {
    const char *source = R"(
      func oneParam(one: Int) -> [Float] {}
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    ASSERT_EQ(func->name, "oneParam");
    ASSERT_EQ(func->params.size(), 1);
    
    ASSERT_EQ(func->params[0].name, "one");
    ASSERT_EQ(func->params[0].ref, ParamRef::val);
    auto *intType = ASSERT_DOWN_CAST(const NamedType, func->params[0].type);
    ASSERT_EQ(intType->name, "Int");
    
    auto *array = ASSERT_DOWN_CAST(const ArrayType, func->ret);
    auto *type = ASSERT_DOWN_CAST(const NamedType, array->elem);
    ASSERT_EQ(type->name, "Float");
  });
  
  TEST(Func - two param, {
    const char *source = R"(
      func swap(first: ref Int, second: ref Int) {}
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    ASSERT_EQ(func->name, "swap");
    ASSERT_EQ(func->params.size(), 2);
    
    ASSERT_EQ(func->params[0].name, "first");
    ASSERT_EQ(func->params[1].name, "second");
    ASSERT_EQ(func->params[0].ref, ParamRef::ref);
    ASSERT_EQ(func->params[1].ref, ParamRef::ref);
  });
  
  TEST(Func - two param (no comma), {
    const char *source = R"(
      func woops(first: String oh_no: Int) {}
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Type - keyword, {
    const char *source = R"(
      type dummy = func;
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Type - Name, {
    const char *source = R"(
      type dummy = name;
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *alias = ASSERT_DOWN_CAST(const TypeAlias, ast.global[0]);
    ASSERT_EQ(alias->name, "dummy");
    
    auto *named = ASSERT_DOWN_CAST(const NamedType, alias->type);
    ASSERT_EQ(named->name, "name");
  });
  
  TEST(Type - Array of functions, {
    const char *source = R"(
      type dummy = [func(ref Int, ref Double) -> Char];
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *alias = ASSERT_DOWN_CAST(const TypeAlias, ast.global[0]);
    ASSERT_EQ(alias->name, "dummy");
    
    auto *array = ASSERT_DOWN_CAST(const ArrayType, alias->type);
    auto *func = ASSERT_DOWN_CAST(const FuncType, array->elem);
    ASSERT_EQ(func->params.size(), 2);
    ASSERT_EQ(func->params[0].ref, ParamRef::ref);
    ASSERT_EQ(func->params[1].ref, ParamRef::ref);
    
    auto *first = ASSERT_DOWN_CAST(const NamedType, func->params[0].type);
    ASSERT_EQ(first->name, "Int");
    auto *second = ASSERT_DOWN_CAST(const NamedType, func->params[1].type);
    ASSERT_EQ(second->name, "Double");
    auto *ret = ASSERT_DOWN_CAST(const NamedType, func->ret);
    ASSERT_EQ(ret->name, "Char");
  });
  
  TEST(Type - Function no ret type, {
    const char *source = R"(
      type dummy = func(Int, Char);
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *alias = ASSERT_DOWN_CAST(const TypeAlias, ast.global[0]);
    ASSERT_EQ(alias->name, "dummy");
    ASSERT_FALSE(alias->strong);
    
    auto *func = ASSERT_DOWN_CAST(const FuncType, alias->type);
    ASSERT_EQ(func->params.size(), 2);
    ASSERT_FALSE(func->ret);
  });
  
  TEST(Type - Invalid, {
    const char *source = R"(
      type dummy = {Int};
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Stat - Block, {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 4);
  });
  
  TEST(Stat - If, {
    const char *source = R"(
      func dummy() {
        if (expr) {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 3);
    
    {
      auto *ifNode = ASSERT_DOWN_CAST(const If, block[0]);
      ASSERT_TRUE(ifNode->cond);
      auto *block = ASSERT_DOWN_CAST(const Block, ifNode->body);
      ASSERT_EQ(block->nodes.size(), 1);
      ASSERT_FALSE(ifNode->elseBody);
    }
    
    {
      auto *ifNode = ASSERT_DOWN_CAST(const If, block[1]);
      ASSERT_TRUE(ifNode->cond);
      auto *block = ASSERT_DOWN_CAST(const Block, ifNode->body);
      ASSERT_EQ(block->nodes.size(), 2);
      auto *elseBlock = ASSERT_DOWN_CAST(const Block, ifNode->elseBody);
      ASSERT_EQ(elseBlock->nodes.size(), 3);
    }
    
    {
      auto *ifNode = ASSERT_DOWN_CAST(const If, block[2]);
      ASSERT_TRUE(ifNode->cond);
      auto *block = ASSERT_DOWN_CAST(const Block, ifNode->body);
      ASSERT_EQ(block->nodes.size(), 4);
      auto *elseIf = ASSERT_DOWN_CAST(const If, ifNode->elseBody);
      ASSERT_TRUE(elseIf->cond);
      auto *elseIfBlock = ASSERT_DOWN_CAST(const Block, elseIf->body);
      ASSERT_EQ(elseIfBlock->nodes.size(), 5);
      auto *elseIfElseBlock = ASSERT_DOWN_CAST(const Block, elseIf->elseBody);
      ASSERT_EQ(elseIfElseBlock->nodes.size(), 6);
    }
  });
  
  TEST(Stat - Keywords, {
    const char *source = R"(
      func dummy() {
        break;
        continue;
        return;
        return expr;
      }
    )";
    
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 4);
    
    ASSERT_DOWN_CAST(const Break, block[0]);
    ASSERT_DOWN_CAST(const Continue, block[1]);
    
    {
      auto *retVoid = ASSERT_DOWN_CAST(const Return, block[2]);
      ASSERT_FALSE(retVoid->expr);
    }
    
    {
      auto *retExpr = ASSERT_DOWN_CAST(const Return, block[3]);
      ASSERT_TRUE(retExpr->expr);
    }
  });
  
  TEST(Stat - Loops, {
    const char *source = R"(
      func dummy() {
        while (expr) {}
        for (i := expr; expr; i++) {}
        for (; expr; i++) {}
        for (; expr; ) {}
      }
    )";
    
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 4);
    
    {
      auto *whileNode = ASSERT_DOWN_CAST(const While, block[0]);
      ASSERT_TRUE(whileNode->cond);
      ASSERT_DOWN_CAST(const Block, whileNode->body);
    }
    {
      auto *forNode = ASSERT_DOWN_CAST(const For, block[1]);
      ASSERT_DOWN_CAST(const DeclAssign, forNode->init);
      ASSERT_TRUE(forNode->cond);
      ASSERT_DOWN_CAST(const IncrDecr, forNode->incr);
      ASSERT_DOWN_CAST(const Block, forNode->body);
    }
    {
      auto *forNode = ASSERT_DOWN_CAST(const For, block[2]);
      ASSERT_FALSE(forNode->init);
      ASSERT_TRUE(forNode->cond);
      ASSERT_DOWN_CAST(const IncrDecr, forNode->incr);
      ASSERT_DOWN_CAST(const Block, forNode->body);
    }
    {
      auto *forNode = ASSERT_DOWN_CAST(const For, block[3]);
      ASSERT_FALSE(forNode->init);
      ASSERT_TRUE(forNode->cond);
      ASSERT_FALSE(forNode->incr);
      ASSERT_DOWN_CAST(const Block, forNode->body);
    }
  });
  
  TEST(Switch - standard, {
    const char *source = R"(
      func dummy() {
        switch (expr) {}
        
        switch (expr) {
          case (expr) {
            expr++;
            continue;
          }
          case (expr) {
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
    
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 2);
    
    {
      auto *switchNode = ASSERT_DOWN_CAST(const Switch, block[0]);
      ASSERT_TRUE(switchNode->expr);
      ASSERT_TRUE(switchNode->cases.empty());
    }
    {
      auto *switchNode = ASSERT_DOWN_CAST(const Switch, block[1]);
      ASSERT_TRUE(switchNode->expr);
      const auto &cases = switchNode->cases;
      ASSERT_EQ(cases.size(), 4);
      {
        ASSERT_TRUE(cases[0].expr);
        auto *block = ASSERT_DOWN_CAST(const Block, cases[0].body);
        ASSERT_TRUE(block->nodes[0]);
        ASSERT_DOWN_CAST(const Continue, block->nodes[1]);
      }
      {
        ASSERT_TRUE(cases[1].expr);
        auto *block = ASSERT_DOWN_CAST(const Block, cases[1].body);
        ASSERT_TRUE(block->nodes[0]);
        ASSERT_DOWN_CAST(const Break, block->nodes[1]);
      }
      {
        ASSERT_TRUE(cases[2].expr);
        auto *block = ASSERT_DOWN_CAST(const Block, cases[2].body);
        ASSERT_TRUE(block->nodes[0]);
        auto *ret = ASSERT_DOWN_CAST(const Return, block->nodes[1]);
        ASSERT_TRUE(ret->expr);
      }
      {
        ASSERT_FALSE(cases[3].expr);
        const Block *block = ASSERT_DOWN_CAST(const Block, cases[3].body);
        ASSERT_TRUE(block->nodes[0]);
        auto *ret = ASSERT_DOWN_CAST(const Return, block->nodes[1]);
        ASSERT_FALSE(ret->expr);
      }
    }
  });
  
  TEST(Switch - no statement, {
    const char *source = R"(
      func dummy() {
        switch (expr) {
          case (expr)
            |; // wait, that's not a statement
        }
      }
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Switch - no case, {
    const char *source = R"(
      func dummy() {
        switch (expr) {
          break;
        }
      }
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Decl - Var and Let, {
    const char *source = R"(
      func dummy() {
        var noInit: Double;
        var initAndType: Int = expr;
        var deducedVar = expr;
        let myLet: MyClass = expr;
        let deducedLet = expr;
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 5);
    
    {
      auto *noInit = ASSERT_DOWN_CAST(const Var, block[0]);
      ASSERT_EQ(noInit->name, "noInit");
      ASSERT_FALSE(noInit->expr);
      auto *noInitType = ASSERT_DOWN_CAST(const NamedType, noInit->type);
      ASSERT_EQ(noInitType->name, "Double");
    }
    
    {
      auto *initAndType = ASSERT_DOWN_CAST(const Var, block[1]);
      ASSERT_EQ(initAndType->name, "initAndType");
      ASSERT_TRUE(initAndType->expr);
      auto *initAndTypeT = ASSERT_DOWN_CAST(const NamedType, initAndType->type);
      ASSERT_EQ(initAndTypeT->name, "Int");
    }
    
    {
      auto *deducedVar = ASSERT_DOWN_CAST(const Var, block[2]);
      ASSERT_EQ(deducedVar->name, "deducedVar");
      ASSERT_TRUE(deducedVar->expr);
      ASSERT_FALSE(deducedVar->type);
    }
    
    {
      auto *myLet = ASSERT_DOWN_CAST(const Let, block[3]);
      ASSERT_EQ(myLet->name, "myLet");
      ASSERT_TRUE(myLet->expr);
      auto *myLetType = ASSERT_DOWN_CAST(const NamedType, myLet->type);
      ASSERT_EQ(myLetType->name, "MyClass");
    }
    
    {
      auto *deducedLet = ASSERT_DOWN_CAST(const Let, block[4]);
      ASSERT_EQ(deducedLet->name, "deducedLet");
      ASSERT_TRUE(deducedLet->expr);
      ASSERT_FALSE(deducedLet->type);
    }
  });
  
  TEST(Decl - extern, {
    const char *source = R"(
      func a() {}
      var b = 0;
      let c = 0;
    
      extern func d() {}
      extern var e = 0;
      extern let f = 0;
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 6);
    
    auto *a = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    ASSERT_EQ(a->name, "a");
    ASSERT_FALSE(a->external);
    
    auto *b = ASSERT_DOWN_CAST(const Var, ast.global[1]);
    ASSERT_EQ(b->name, "b");
    ASSERT_FALSE(b->external);
    
    auto *c = ASSERT_DOWN_CAST(const Let, ast.global[2]);
    ASSERT_EQ(c->name, "c");
    ASSERT_FALSE(c->external);
    
    auto *d = ASSERT_DOWN_CAST(const Func, ast.global[3]);
    ASSERT_EQ(d->name, "d");
    ASSERT_TRUE(d->external);
    
    auto *e = ASSERT_DOWN_CAST(const Var, ast.global[4]);
    ASSERT_EQ(e->name, "e");
    ASSERT_TRUE(e->external);
    
    auto *f = ASSERT_DOWN_CAST(const Let, ast.global[5]);
    ASSERT_EQ(f->name, "f");
    ASSERT_TRUE(f->external);
  });
  
  TEST(Decl - extern type, {
    const char *source = R"(
      extern type sint;
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Decl - extern local, {
    const char *source = R"(
      func fn() {
        extern var num = 0;
      }
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Decl - global, {
    const char *source = R"(
      module my_awesome_module;
    
      ; // an extra semicolon never hurt anyone
    
      if (expr) {} // that's not a declaration
    )";
    // good test for the color logger because all priorities are used
    // can't use color logger for everything because Xcode Console doesn't
    // support color. ;-(
    ColorSink colorLog;
    ASSERT_THROWS(createAST(source, colorLog), FatalError);
  });
  
  TEST(Struct - empty, {
    const char *source = R"(
      type NoMembers struct {};
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *alias = ASSERT_DOWN_CAST(const TypeAlias, ast.global[0]);
    ASSERT_EQ(alias->name, "NoMembers");
    ASSERT_TRUE(alias->strong);
    auto *strut = ASSERT_DOWN_CAST(const StructType, alias->type);
    ASSERT_TRUE(strut->fields.empty());
  });

  TEST(Struct - Vars, {
    const char *source = R"(
      type Vec2 struct {
        x: Float;
        y: Float;
      };
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *alias = ASSERT_DOWN_CAST(const TypeAlias, ast.global[0]);
    ASSERT_EQ(alias->name, "Vec2");
    auto *strut = ASSERT_DOWN_CAST(const StructType, alias->type);
    ASSERT_EQ(strut->fields.size(), 2);
    
    {
      const Field &field = strut->fields[0];
      ASSERT_EQ(field.name, "x");
    }
    {
      const Field &field = strut->fields[1];
      ASSERT_EQ(field.name, "y");
    }
  });
  
  TEST(Struct - Functions, {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 2);
    auto *alias = ASSERT_DOWN_CAST(const TypeAlias, ast.global[0]);
    ASSERT_EQ(alias->name, "Vec2");
    auto *strut = ASSERT_DOWN_CAST(const StructType, alias->type);
    ASSERT_EQ(strut->fields.size(), 2);
    
    {
      const Field &field = strut->fields[0];
      ASSERT_EQ(field.name, "x");
    }
    {
      const Field &field = strut->fields[1];
      ASSERT_EQ(field.name, "y");
    }
    
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[1]);
    ASSERT_EQ(func->name, "add");
    ASSERT_TRUE(func->receiver);
    ASSERT_EQ(func->params.size(), 1);
    ASSERT_FALSE(func->ret);
  });
  
  TEST(Struct - Bad member, {
    const char *source = R"(
      struct C {
        int x;
      };
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Expr - Simple literals, {
    const char *source = R"(
      func dummy() {
        var a = "This is a string";
        var a = 'c';
        var a = 73;
        var a = true;
        var a = false;
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 5);
    
    {
      auto *var = ASSERT_DOWN_CAST(const Var, block[0]);
      auto *lit = ASSERT_DOWN_CAST(const StringLiteral, var->expr);
      ASSERT_EQ(lit->value, "This is a string");
    }
    {
      auto *var = ASSERT_DOWN_CAST(const Var, block[1]);
      auto *lit = ASSERT_DOWN_CAST(const CharLiteral, var->expr);
      ASSERT_EQ(lit->value, "c");
    }
    {
      auto *var = ASSERT_DOWN_CAST(const Var, block[2]);
      auto *lit = ASSERT_DOWN_CAST(const NumberLiteral, var->expr);
      ASSERT_EQ(lit->value, "73");
    }
    {
      auto *var = ASSERT_DOWN_CAST(const Var, block[3]);
      auto *lit = ASSERT_DOWN_CAST(const BoolLiteral, var->expr);
      ASSERT_EQ(lit->value, true);
    }
    {
      auto *var = ASSERT_DOWN_CAST(const Var, block[4]);
      auto *lit = ASSERT_DOWN_CAST(const BoolLiteral, var->expr);
      ASSERT_EQ(lit->value, false);
    }
  });
  
  TEST(Expr - Array, {
    const char *source = R"(
      func dummy() {
        var empty = [];
        var seven = [7];
        var seven_ate_nine = [7, 8, 9];
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 3);
    
    {
      auto *var = ASSERT_DOWN_CAST(const Var, block[0]);
      auto *lit = ASSERT_DOWN_CAST(const ArrayLiteral, var->expr);
      ASSERT_TRUE(lit->exprs.empty());
    }
    {
      auto *var = ASSERT_DOWN_CAST(const Var, block[1]);
      auto *lit = ASSERT_DOWN_CAST(const ArrayLiteral, var->expr);
      ASSERT_EQ(lit->exprs.size(), 1);
      auto *seven = ASSERT_DOWN_CAST(const NumberLiteral, lit->exprs[0]);
      ASSERT_EQ(seven->value, "7");
    }
    {
      auto *var = ASSERT_DOWN_CAST(const Var, block[2]);
      auto *lit = ASSERT_DOWN_CAST(const ArrayLiteral, var->expr);
      ASSERT_EQ(lit->exprs.size(), 3);
      auto *seven = ASSERT_DOWN_CAST(const NumberLiteral, lit->exprs[0]);
      ASSERT_EQ(seven->value, "7");
      auto *eight = ASSERT_DOWN_CAST(const NumberLiteral, lit->exprs[1]);
      ASSERT_EQ(eight->value, "8");
      auto *nine = ASSERT_DOWN_CAST(const NumberLiteral, lit->exprs[2]);
      ASSERT_EQ(nine->value, "9");
    }
  });
  
  TEST(Expr - Init List, {
    const char *source = R"(
      let vec: Vec2 = {2.0, 3.0};
      let zero: sint = {};
      let abc = make Tup3 {"a", "b", "c"};
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 3);
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[0]);
      ASSERT_EQ(let->name, "vec");
        auto *type = ASSERT_DOWN_CAST(const NamedType, let->type);
          ASSERT_EQ(type->name, "Vec2");
        auto *list = ASSERT_DOWN_CAST(const InitList, let->expr);
          ASSERT_EQ(list->exprs.size(), 2);
          IS_NUM(list->exprs[0], "2.0");
          IS_NUM(list->exprs[1], "3.0");
    }
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[1]);
      ASSERT_EQ(let->name, "zero");
        auto *type = ASSERT_DOWN_CAST(const NamedType, let->type);
          ASSERT_EQ(type->name, "sint");
        auto *list = ASSERT_DOWN_CAST(const InitList, let->expr);
          ASSERT_TRUE(list->exprs.empty());
    }
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[2]);
      ASSERT_EQ(let->name, "abc");
        ASSERT_FALSE(let->type);
        auto *make = ASSERT_DOWN_CAST(const Make, let->expr);
          auto *type = ASSERT_DOWN_CAST(const NamedType, make->type);
            ASSERT_EQ(type->name, "Tup3");
          auto *list = ASSERT_DOWN_CAST(const InitList, make->expr);
            ASSERT_EQ(list->exprs.size(), 3);
            auto *a = ASSERT_DOWN_CAST(const StringLiteral, list->exprs[0]);
              ASSERT_EQ(a->value, "a");
            auto *b = ASSERT_DOWN_CAST(const StringLiteral, list->exprs[1]);
              ASSERT_EQ(b->value, "b");
            auto *c = ASSERT_DOWN_CAST(const StringLiteral, list->exprs[2]);
              ASSERT_EQ(c->value, "c");
    }
  });
  
  TEST(Expr - Lambda, {
    const char *source = R"(
      let nothing = func() {};
      let identity = func(n: Int) -> Int {
        return expr;
      };
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 2);
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[0]);
      auto *lambda = ASSERT_DOWN_CAST(const Lambda, let->expr);
      ASSERT_TRUE(lambda->params.empty());
      ASSERT_FALSE(lambda->ret);
      ASSERT_TRUE(lambda->body.nodes.empty());
    }
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[1]);
      auto *lambda = ASSERT_DOWN_CAST(const Lambda, let->expr);
      ASSERT_EQ(lambda->params.size(), 1);
      
      ASSERT_EQ(lambda->params[0].name, "n");
      auto *n = ASSERT_DOWN_CAST(const NamedType, lambda->params[0].type);
      ASSERT_EQ(n->name, "Int");
      
      ASSERT_EQ(lambda->body.nodes.size(), 1);
      auto *returnStat = ASSERT_DOWN_CAST(const Return, lambda->body.nodes[0]);
      ASSERT_TRUE(returnStat->expr);
    }
  });
  
  TEST(Expr - Identifier, {
    const char *source = R"(
      let five = 5;
      let alsoFive = five;
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 2);
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[0]);
      ASSERT_EQ(let->name, "five");
      auto *num = ASSERT_DOWN_CAST(const NumberLiteral, let->expr);
      ASSERT_EQ(num->value, "5");
    }
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[1]);
      ASSERT_EQ(let->name, "alsoFive");
      auto *ident = ASSERT_DOWN_CAST(const Identifier, let->expr);
      ASSERT_EQ(ident->name, "five");
    }
  });
  
  TEST(Expr - Make, {
    const char *source = R"(
      let five = make uint make sint 5.0;
      let six = make real (6);
      let one = make sint !false;
      let two = make real 6 / 3.0;
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 4);
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[0]);
      ASSERT_EQ(let->name, "five");
      auto *make_uint = ASSERT_DOWN_CAST(const Make, let->expr);
        auto *uint = ASSERT_DOWN_CAST(const NamedType, make_uint->type);
          ASSERT_EQ(uint->name, "uint");
        auto *make_sint = ASSERT_DOWN_CAST(const Make, make_uint->expr);
          auto *sint = ASSERT_DOWN_CAST(const NamedType, make_sint->type);
            ASSERT_EQ(sint->name, "sint");
          IS_NUM(make_sint->expr, "5.0");
    }
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[1]);
      ASSERT_EQ(let->name, "six");
      auto *make_real = ASSERT_DOWN_CAST(const Make, let->expr);
        auto *real = ASSERT_DOWN_CAST(const NamedType, make_real->type);
          ASSERT_EQ(real->name, "real");
        IS_NUM(make_real->expr, "6");
    }
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[2]);
      ASSERT_EQ(let->name, "one");
      auto *make_sint = ASSERT_DOWN_CAST(const Make, let->expr);
        auto *sint = ASSERT_DOWN_CAST(const NamedType, make_sint->type);
          ASSERT_EQ(sint->name, "sint");
        auto *bnot = IS_UOP(make_sint->expr, bool_not);
          auto *boolLit = ASSERT_DOWN_CAST(const BoolLiteral, bnot->expr);
            ASSERT_FALSE(boolLit->value);
    }
    
    {
      auto *let = ASSERT_DOWN_CAST(const Let, ast.global[3]);
      ASSERT_EQ(let->name, "two");
      auto *div = IS_BOP(let->expr, div);
        auto *make_real = ASSERT_DOWN_CAST(const Make, div->left);
          auto *real = ASSERT_DOWN_CAST(const NamedType, make_real->type);
            ASSERT_EQ(real->name, "real");
          IS_NUM(make_real->expr, "6");
        IS_NUM(div->right, "3.0");
    }
  });
  
  TEST(Expr - math, {
    /*
    
      add
     /   \
    3    div
       /     \
    mul       pow1
   /   \     /    \
  4     2 sub      pow2
         /   \    /    \
        1    mod 2      3
            /   \
           5     6
    */
  
    const char *source = "let a = 3 + 4 * 2 / (1 - 5 % 6) ** 2 ** 3;";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *let = ASSERT_DOWN_CAST(const Let, ast.global[0]);
      auto *add = IS_BOP(let->expr, add);
        IS_NUM(add->left, "3");
        auto *div = IS_BOP(add->right, div);
          auto *mul = IS_BOP(div->left, mul);
            IS_NUM(mul->left, "4");
            IS_NUM(mul->right, "2");
          auto *pow1 = IS_BOP(div->right, pow);
            auto *sub = IS_BOP(pow1->left, sub);
              IS_NUM(sub->left, "1");
              auto *mod = IS_BOP(sub->right, mod);
                IS_NUM(mod->left, "5");
                IS_NUM(mod->right, "6");
            auto *pow2 = IS_BOP(pow1->right, pow);
              IS_NUM(pow2->left, "2");
              IS_NUM(pow2->right, "3");
  });
  
  TEST(Expr - assign and compare, {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 1);
    
    auto *assign_add = IS_AOP(block[0], add);
      IS_ID(assign_add->left, "yeah");
      auto *bool_and = IS_BOP(assign_add->right, bool_and);
        auto *eq = IS_BOP(bool_and->left, eq);
          IS_NUM(eq->left, "1");
          IS_ID(eq->right, "two");
        auto *lt = IS_BOP(bool_and->right, lt);
          IS_ID(lt->left, "four");
          IS_NUM(lt->right, "5");
  });
  
  TEST(Expr - unary, {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 3);
    
    auto *incr = ASSERT_DOWN_CAST(const IncrDecr, block[0]);
    ASSERT_TRUE(incr->incr);
    auto *decr = ASSERT_DOWN_CAST(const IncrDecr, block[1]);
    ASSERT_FALSE(decr->incr);
    auto *assign = IS_AOP(block[2], sub);
      IS_ID(assign->left, "n");
      auto *sub = IS_BOP(assign->right, sub);
        auto *lneg = IS_UOP(sub->left, neg);
          IS_ID(lneg->expr, "n");
        auto *rneg = IS_UOP(sub->right, neg);
          auto *add = IS_BOP(rneg->expr, add);
            auto *bitNot = IS_UOP(add->left, bit_not);
              IS_NUM(bitNot->expr, "5");
            auto *boolNot = IS_UOP(add->right, bool_not);
              IS_NUM(boolNot->expr, "0");
  });
  
  TEST(Expr - ternary, {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 1);
    
    auto *assignMul = IS_AOP(block[0], mul);
      IS_ID(assignMul->left, "e");
      auto *tern = ASSERT_DOWN_CAST(const Ternary, assignMul->right);
        auto *le = IS_BOP(tern->cond, le);
          IS_ID(le->left, "a");
          IS_ID(le->right, "d");
        IS_ID(tern->troo, "a");
        auto *div = IS_BOP(tern->fols, div);
          IS_ID(div->left, "a");
          IS_ID(div->right, "d");
  });
  
  TEST(Expr - member and subscript, {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 1);
    
    auto *let = ASSERT_DOWN_CAST(const Let, block[0]);
    auto *sub1 = ASSERT_DOWN_CAST(const Subscript, let->expr);
      auto *sub2 = ASSERT_DOWN_CAST(const Subscript, sub1->object);
        auto *mem1 = ASSERT_DOWN_CAST(const MemberIdent, sub2->object);
          auto *mem2 = ASSERT_DOWN_CAST(const MemberIdent, mem1->object);
            IS_ID(mem2->object, "a");
            ASSERT_EQ(mem2->member, "b");
          ASSERT_EQ(mem1->member, "c");
        auto *sub3 = ASSERT_DOWN_CAST(const Subscript, sub2->index);
          auto *mem3 = ASSERT_DOWN_CAST(const MemberIdent, sub3->object);
            IS_ID(mem3->object, "e");
            ASSERT_EQ(mem3->member, "f");
          IS_ID(sub3->index, "g");
      IS_ID(sub1->index, "h");
  });
  
  TEST(Expr - bits, {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 1);
    auto *let = ASSERT_DOWN_CAST(const Let, block[0]);
    
    auto *boolOr = IS_BOP(let->expr, bool_or);
      auto *ne = IS_BOP(boolOr->left, ne);
        auto *bnot = IS_UOP(ne->left, bit_not);
          IS_ID(bnot->expr, "a");
        auto *shr = IS_BOP(ne->right, bit_shr);
          auto *shl = IS_BOP(shr->left, bit_shl);
            IS_ID(shl->left, "b");
            IS_NUM(shl->right, "2");
          IS_NUM(shr->right, "3");
      auto *bor = IS_BOP(boolOr->right, bit_or);
        auto *band = IS_BOP(bor->left, bit_and);
          IS_ID(band->left, "c");
          IS_ID(band->right, "d");
        auto *bxor = IS_BOP(bor->right, bit_xor);
          IS_ID(bxor->left, "e");
          IS_ID(bxor->right, "f");
  });
  
  TEST(Expr - function call, {
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
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 1);
    
    auto *assign = ASSERT_DOWN_CAST(const CallAssign, block[0]);
    auto *call1 = &assign->call;
      auto *mem1 = ASSERT_DOWN_CAST(const MemberIdent, call1->func);
        auto *sub = ASSERT_DOWN_CAST(const Subscript, mem1->object);
          IS_ID(sub->object, "a");
          IS_NUM(sub->index, "2");
        ASSERT_EQ(mem1->member, "b");
      ASSERT_EQ(call1->args.size(), 2);
        auto *call2 = ASSERT_DOWN_CAST(const FuncCall, call1->args[0]);
          auto *mem2 = ASSERT_DOWN_CAST(const MemberIdent, call2->func);
            auto *call3 = ASSERT_DOWN_CAST(const FuncCall, mem2->object);
              IS_ID(call3->func, "c");
              ASSERT_EQ(call3->args.size(), 1);
                IS_NUM(call3->args[0], "3");
            ASSERT_EQ(mem2->member, "d");
          ASSERT_TRUE(call2->args.empty());
        IS_NUM(call1->args[1], "4");
  });
  
  TEST(Expr - assign, {
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
        a **= b;
        a <<= b;
        a >>= b;
        a &= b;
        a ^= b;
        a |= b > c >= d;
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 7);
    
    auto *mod = IS_AOP(block[0], mod);
      IS_ID(mod->left, "a");
      IS_ID(mod->right, "b");
    auto *pow = IS_AOP(block[1], pow);
      IS_ID(pow->left, "a");
      IS_ID(pow->right, "b");
    auto *shl = IS_AOP(block[2], bit_shl);
      IS_ID(shl->left, "a");
      IS_ID(shl->right, "b");
    auto *shr = IS_AOP(block[3], bit_shr);
      IS_ID(shr->left, "a");
      IS_ID(shr->right, "b");
    auto *band = IS_AOP(block[4], bit_and);
      IS_ID(band->left, "a");
      IS_ID(band->right, "b");
    auto *bxor = IS_AOP(block[5], bit_xor);
      IS_ID(bxor->left, "a");
      IS_ID(bxor->right, "b");
    auto *bor = IS_AOP(block[6], bit_or);
      IS_ID(bor->left, "a");
      auto *ge = IS_BOP(bor->right, ge);
        auto *gt = IS_BOP(ge->left, gt);
          IS_ID(gt->left, "b");
          IS_ID(gt->right, "c");
        IS_ID(ge->right, "d");
  });
  
  TEST(Expr - Result unused, {
    const char *source = R"(
      func dummy() {
        a.b.c;
      }
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Expr - Unary +, {
    const char *source = R"(
      func dummy() {
        let a = +;
      }
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Factorial, {
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
      func fac(n: Int) {
        return n == 0 ? 1 : n * fac(n - 1);
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0]);
    ASSERT_EQ(func->name, "fac");
    ASSERT_EQ(func->params.size(), 1);
      ASSERT_EQ(func->params[0].name, "n");
      ASSERT_EQ(func->params[0].ref, ParamRef::val);
      auto *inttype = ASSERT_DOWN_CAST(const NamedType, func->params[0].type);
        ASSERT_EQ(inttype->name, "Int");
    ASSERT_FALSE(func->ret);
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 1);
    auto *ret = ASSERT_DOWN_CAST(const Return, block[0]);
      auto *ternary = ASSERT_DOWN_CAST(const Ternary, ret->expr);
        auto *eq = IS_BOP(ternary->cond, eq);
          IS_ID(eq->left, "n");
          IS_NUM(eq->right, "0");
        IS_NUM(ternary->troo, "1");
        auto *mul = IS_BOP(ternary->fols, mul);
          IS_ID(mul->left, "n");
          auto *call = ASSERT_DOWN_CAST(const FuncCall, mul->right);
            IS_ID(call->func, "fac");
            ASSERT_EQ(call->args.size(), 1);
              auto *sub = IS_BOP(call->args[0], sub);
                IS_ID(sub->left, "n");
                IS_NUM(sub->right, "1");
  });
})
