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
#include <STELA/lexical analysis.hpp>

using namespace stela;
using namespace stela::ast;

TEST_GROUP(Syntax, {
  StreamLog log;

  TEST(No tokens, {
    const AST ast = createAST(Tokens{}, log);
    ASSERT_TRUE(ast.global.empty());
  });
  
  TEST(Enum - empty, {
    const char *source = R"(
      enum NoCases {}
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *enumNode = ASSERT_DOWN_CAST(const Enum, ast.global[0].get());
    ASSERT_EQ(enumNode->name, "NoCases");
    ASSERT_TRUE(enumNode->cases.empty());
  });
  
  TEST(Enum - standard, {
    const char *source = R"(
      enum Dir {
        up,
        right,
        down,
        left
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *enumNode = ASSERT_DOWN_CAST(const Enum, ast.global[0].get());
    ASSERT_EQ(enumNode->name, "Dir");
    ASSERT_EQ(enumNode->cases.size(), 4);
    
    ASSERT_EQ(enumNode->cases[0].name, "up");
    ASSERT_EQ(enumNode->cases[1].name, "right");
    ASSERT_EQ(enumNode->cases[2].name, "down");
    ASSERT_EQ(enumNode->cases[3].name, "left");
  });
  
  TEST(Enum - extra comma, {
    const char *source = R"(
      enum ExtraComma {
        first,
        second,
      }
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Enum - missing comma, {
    const char *source = R"(
      enum ExtraComma {
        first
        oh_no
      }
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Func - empty, {
    const char *source = R"(
      func empty() {}
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    ASSERT_EQ(func->name, "empty");
    ASSERT_TRUE(func->params.empty());
  });
  
  TEST(Func - one param, {
    const char *source = R"(
      func oneParam(one: Int) -> [Float] {}
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    ASSERT_EQ(func->name, "oneParam");
    ASSERT_EQ(func->params.size(), 1);
    
    ASSERT_EQ(func->params[0].name, "one");
    ASSERT_EQ(func->params[0].ref, ParamRef::value);
    
    auto *array = ASSERT_DOWN_CAST(const ArrayType, func->ret.get());
    auto *type = ASSERT_DOWN_CAST(const NamedType, array->elem.get());
    ASSERT_EQ(type->name, "Float");
  });
  
  TEST(Func - two param, {
    const char *source = R"(
      func swap(first: inout Int, second: inout Int) {}
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    ASSERT_EQ(func->name, "swap");
    ASSERT_EQ(func->params.size(), 2);
    
    ASSERT_EQ(func->params[0].name, "first");
    ASSERT_EQ(func->params[1].name, "second");
    ASSERT_EQ(func->params[0].ref, ParamRef::inout);
    ASSERT_EQ(func->params[1].ref, ParamRef::inout);
  });
  
  TEST(Func - two param (no comma), {
    const char *source = R"(
      func woops(first: String oh_no: Int) {}
    )";
    ASSERT_THROWS(createAST(source, log), FatalError);
  });
  
  TEST(Type - Array of functions, {
    const char *source = R"(
      typealias dummy = [(inout Int, inout Double) -> Void];
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *alias = ASSERT_DOWN_CAST(const TypeAlias, ast.global[0].get());
    ASSERT_EQ(alias->name, "dummy");
    
    auto *array = ASSERT_DOWN_CAST(const ArrayType, alias->type.get());
    auto *func = ASSERT_DOWN_CAST(const FuncType, array->elem.get());
    ASSERT_EQ(func->params.size(), 2);
    ASSERT_EQ(func->params[0].ref, ParamRef::inout);
    ASSERT_EQ(func->params[1].ref, ParamRef::inout);
    
    auto *first = ASSERT_DOWN_CAST(const NamedType, func->params[0].type.get());
    ASSERT_EQ(first->name, "Int");
    auto *second = ASSERT_DOWN_CAST(const NamedType, func->params[1].type.get());
    ASSERT_EQ(second->name, "Double");
    auto *ret = ASSERT_DOWN_CAST(const NamedType, func->ret.get());
    ASSERT_EQ(ret->name, "Void");
  });
  
  TEST(Type - Dictionary, {
    const char *source = R"(
      typealias dummy = [String: [Int]];
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *alias = ASSERT_DOWN_CAST(const TypeAlias, ast.global[0].get());
    ASSERT_EQ(alias->name, "dummy");
    
    auto *dict = ASSERT_DOWN_CAST(const DictType, alias->type.get());
    auto *key = ASSERT_DOWN_CAST(const NamedType, dict->key.get());
    ASSERT_EQ(key->name, "String");
    auto *val = ASSERT_DOWN_CAST(const ArrayType, dict->val.get());
    auto *valElem = ASSERT_DOWN_CAST(const NamedType, val->elem.get());
    ASSERT_EQ(valElem->name, "Int");
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
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    auto *blockNode = ASSERT_DOWN_CAST(const Block, func->body.get());
    const auto &block = blockNode->nodes;
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
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    auto *blockNode = ASSERT_DOWN_CAST(const Block, func->body.get());
    const auto &block = blockNode->nodes;
    ASSERT_EQ(block.size(), 3);
    
    {
      auto *ifNode = ASSERT_DOWN_CAST(const If, block[0].get());
      ASSERT_TRUE(ifNode->cond);
      auto *block = ASSERT_DOWN_CAST(const Block, ifNode->body.get());
      ASSERT_EQ(block->nodes.size(), 1);
      ASSERT_FALSE(ifNode->elseBody);
    }
    
    {
      auto *ifNode = ASSERT_DOWN_CAST(const If, block[1].get());
      ASSERT_TRUE(ifNode->cond);
      auto *block = ASSERT_DOWN_CAST(const Block, ifNode->body.get());
      ASSERT_EQ(block->nodes.size(), 2);
      auto *elseBlock = ASSERT_DOWN_CAST(const Block, ifNode->elseBody.get());
      ASSERT_EQ(elseBlock->nodes.size(), 3);
    }
    
    {
      auto *ifNode = ASSERT_DOWN_CAST(const If, block[2].get());
      ASSERT_TRUE(ifNode->cond);
      auto *block = ASSERT_DOWN_CAST(const Block, ifNode->body.get());
      ASSERT_EQ(block->nodes.size(), 4);
      auto *elseIf = ASSERT_DOWN_CAST(const If, ifNode->elseBody.get());
      ASSERT_TRUE(elseIf->cond);
      auto *elseIfBlock = ASSERT_DOWN_CAST(const Block, elseIf->body.get());
      ASSERT_EQ(elseIfBlock->nodes.size(), 5);
      auto *elseIfElseBlock = ASSERT_DOWN_CAST(const Block, elseIf->elseBody.get());
      ASSERT_EQ(elseIfElseBlock->nodes.size(), 6);
    }
  });
  
  TEST(Stat - Keywords, {
    const char *source = R"(
      func dummy() {
        break;
        continue;
        fallthrough;
        return;
        return expr;
      }
    )";
    
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    auto *blockNode = ASSERT_DOWN_CAST(const Block, func->body.get());
    const auto &block = blockNode->nodes;
    ASSERT_EQ(block.size(), 5);
    
    ASSERT_DOWN_CAST(const Break, block[0].get());
    ASSERT_DOWN_CAST(const Continue, block[1].get());
    ASSERT_DOWN_CAST(const Fallthrough, block[2].get());
    
    {
      auto *retVoid = ASSERT_DOWN_CAST(const Return, block[3].get());
      ASSERT_FALSE(retVoid->expr);
    }
    
    {
      auto *retExpr = ASSERT_DOWN_CAST(const Return, block[4].get());
      ASSERT_TRUE(retExpr->expr);
    }
  });
  
  TEST(Stat - Loops, {
    const char *source = R"(
      func dummy() {
        while (expr) {}
        
        repeat {} while (expr);
        
        for (let initial = expr; expr; expr) {}
        for (; expr; expr) {}
        for (; expr; ) {}
      }
    )";
    
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    auto *blockNode = ASSERT_DOWN_CAST(const Block, func->body.get());
    const auto &block = blockNode->nodes;
    ASSERT_EQ(block.size(), 5);
    
    {
      auto *whileNode = ASSERT_DOWN_CAST(const While, block[0].get());
      ASSERT_TRUE(whileNode->cond);
      ASSERT_DOWN_CAST(const Block, whileNode->body.get());
    }
    {
      auto *repeatWhile = ASSERT_DOWN_CAST(const RepeatWhile, block[1].get());
      ASSERT_DOWN_CAST(const Block, repeatWhile->body.get());
      ASSERT_TRUE(repeatWhile->cond);
    }
    {
      auto *forNode = ASSERT_DOWN_CAST(const For, block[2].get());
      ASSERT_DOWN_CAST(const Let, forNode->init.get());
      ASSERT_TRUE(forNode->cond);
      ASSERT_TRUE(forNode->incr);
      ASSERT_DOWN_CAST(const Block, forNode->body.get());
    }
    {
      auto *forNode = ASSERT_DOWN_CAST(const For, block[3].get());
      ASSERT_FALSE(forNode->init);
      ASSERT_TRUE(forNode->cond);
      ASSERT_TRUE(forNode->incr);
      ASSERT_DOWN_CAST(const Block, forNode->body.get());
    }
    {
      auto *forNode = ASSERT_DOWN_CAST(const For, block[4].get());
      ASSERT_FALSE(forNode->init);
      ASSERT_TRUE(forNode->cond);
      ASSERT_FALSE(forNode->incr);
      ASSERT_DOWN_CAST(const Block, forNode->body.get());
    }
  });
  
  TEST(Stat - Switch, {
    const char *source = R"(
      func dummy() {
        switch (expr) {}
        
        switch (expr) {
          case expr:
            expr;
            fallthrough;
          case expr:
            expr;
            break;
          case expr:
            expr;
            return expr;
          default:
            expr;
            return;
        }
      }
    )";
    
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    auto *blockNode = ASSERT_DOWN_CAST(const Block, func->body.get());
    const auto &block = blockNode->nodes;
    ASSERT_EQ(block.size(), 2);
    
    {
      auto *switchNode = ASSERT_DOWN_CAST(const Switch, block[0].get());
      ASSERT_TRUE(switchNode->expr);
      ASSERT_TRUE(switchNode->body.nodes.empty());
    }
    {
      auto *switchNode = ASSERT_DOWN_CAST(const Switch, block[1].get());
      ASSERT_TRUE(switchNode->expr);
      const auto &body = switchNode->body.nodes;
      ASSERT_EQ(body.size(), 12);
      ASSERT_DOWN_CAST(const SwitchCase, body[0].get());
      ASSERT_TRUE(body[1].get());
      ASSERT_DOWN_CAST(const Fallthrough, body[2].get());
      ASSERT_DOWN_CAST(const SwitchCase, body[3].get());
      ASSERT_TRUE(body[4].get());
      ASSERT_DOWN_CAST(const Break, body[5].get());
      ASSERT_DOWN_CAST(const SwitchCase, body[6].get());
      ASSERT_TRUE(body[7].get());
      ASSERT_DOWN_CAST(const Return, body[8].get());
      ASSERT_DOWN_CAST(const SwitchDefault, body[9].get());
      ASSERT_TRUE(body[10].get());
      ASSERT_DOWN_CAST(const Return, body[11].get());
    }
  });
  
  TEST(Decl - Var and Let, {
    const char *source = R"(
      func dummy() {
        var noInit: Double;
        var initAndType: Int = expr;
        var deducedVar = expr;
        let myLet: String = expr;
        let deducedLet = expr;
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    auto *blockNode = ASSERT_DOWN_CAST(const Block, func->body.get());
    const auto &block = blockNode->nodes;
    ASSERT_EQ(block.size(), 5);
    
    {
      auto *noInit = ASSERT_DOWN_CAST(const Var, block[0].get());
      ASSERT_EQ(noInit->name, "noInit");
      ASSERT_FALSE(noInit->expr);
      auto *noInitType = ASSERT_DOWN_CAST(const NamedType, noInit->type.get());
      ASSERT_EQ(noInitType->name, "Double");
    }
    
    {
      auto *initAndType = ASSERT_DOWN_CAST(const Var, block[1].get());
      ASSERT_EQ(initAndType->name, "initAndType");
      ASSERT_TRUE(initAndType->expr);
      auto *initAndTypeT = ASSERT_DOWN_CAST(const NamedType, initAndType->type.get());
      ASSERT_EQ(initAndTypeT->name, "Int");
    }
    
    {
      auto *deducedVar = ASSERT_DOWN_CAST(const Var, block[2].get());
      ASSERT_EQ(deducedVar->name, "deducedVar");
      ASSERT_TRUE(deducedVar->expr);
      ASSERT_FALSE(deducedVar->type);
    }
    
    {
      auto *myLet = ASSERT_DOWN_CAST(const Let, block[3].get());
      ASSERT_EQ(myLet->name, "myLet");
      ASSERT_TRUE(myLet->expr);
      auto *myLetType = ASSERT_DOWN_CAST(const NamedType, myLet->type.get());
      ASSERT_EQ(myLetType->name, "String");
    }
    
    {
      auto *deducedLet = ASSERT_DOWN_CAST(const Let, block[4].get());
      ASSERT_EQ(deducedLet->name, "deducedLet");
      ASSERT_TRUE(deducedLet->expr);
      ASSERT_FALSE(deducedLet->type);
    }
  });
  
  TEST(Struct - empty, {
    const char *source = R"(
      struct NoMembers {}
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *structNode = ASSERT_DOWN_CAST(const Struct, ast.global[0].get());
    ASSERT_EQ(structNode->name, "NoMembers");
    ASSERT_TRUE(structNode->body.empty());
  });

  TEST(Struct - Vars, {
    const char *source = R"(
      struct Vec2 {
        private var x: Float;
        private var y: Float;
        
        static let zero = expr;
        public static let zilch = expr;
        
        let mem = expr;
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *structNode = ASSERT_DOWN_CAST(const Struct, ast.global[0].get());
    ASSERT_EQ(structNode->name, "Vec2");
    ASSERT_EQ(structNode->body.size(), 5);
    
    {
      const Member &mem = structNode->body[0];
      ASSERT_EQ(mem.access, MemAccess::private_);
      ASSERT_EQ(mem.scope, MemScope::member);
      ASSERT_DOWN_CAST(const Var, mem.node.get());
    }
    {
      const Member &mem = structNode->body[1];
      ASSERT_EQ(mem.access, MemAccess::private_);
      ASSERT_EQ(mem.scope, MemScope::member);
      ASSERT_DOWN_CAST(const Var, mem.node.get());
    }
    {
      const Member &mem = structNode->body[2];
      ASSERT_EQ(mem.access, MemAccess::public_);
      ASSERT_EQ(mem.scope, MemScope::static_);
      ASSERT_DOWN_CAST(const Let, mem.node.get());
    }
    {
      const Member &mem = structNode->body[3];
      ASSERT_EQ(mem.access, MemAccess::public_);
      ASSERT_EQ(mem.scope, MemScope::static_);
      ASSERT_DOWN_CAST(const Let, mem.node.get());
    }
    {
      const Member &mem = structNode->body[4];
      ASSERT_EQ(mem.access, MemAccess::public_);
      ASSERT_EQ(mem.scope, MemScope::member);
      ASSERT_DOWN_CAST(const Let, mem.node.get());
    }
  });
  
  TEST(Struct - Functions, {
    const char *source = R"(
      struct Vec2 {
        func add(other: Vec2) {
          expr;
          expr;
        }
        
        static func add(a: Vec2, b: Vec2) {
          return expr;
        }
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *structNode = ASSERT_DOWN_CAST(const Struct, ast.global[0].get());
    ASSERT_EQ(structNode->name, "Vec2");
    ASSERT_EQ(structNode->body.size(), 2);
    
    {
      const Member &mem = structNode->body[0];
      ASSERT_EQ(mem.access, MemAccess::public_);
      ASSERT_EQ(mem.scope, MemScope::member);
      ASSERT_DOWN_CAST(const Func, mem.node.get());
    }
    {
      const Member &mem = structNode->body[1];
      ASSERT_EQ(mem.access, MemAccess::public_);
      ASSERT_EQ(mem.scope, MemScope::static_);
      ASSERT_DOWN_CAST(const Func, mem.node.get());
    }
  });
});
