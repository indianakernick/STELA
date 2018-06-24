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
        second
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
  
  TEST(Stat - Var, {
    const char *source = R"(
      func dummy() {
        var noInit: Double;
        var initAndType: Int = ident;
        var deducedVar = ident;
        let myLet: Double = ident;
        let deducedLet = ident;
      }
    )";
    const AST ast = createAST(source, log);
    ASSERT_EQ(ast.global.size(), 1);
    auto *func = ASSERT_DOWN_CAST(const Func, ast.global[0].get());
    const auto &block = func->body.nodes;
    ASSERT_EQ(block.size(), 5);
    
    auto *noInit = ASSERT_DOWN_CAST(const Var, block[0].get());
    ASSERT_EQ(noInit->name, "noInit");
    ASSERT_EQ(noInit->expr, nullptr);
    auto *noInitType = ASSERT_DOWN_CAST(const NamedType, noInit->type.get());
    ASSERT_EQ(noInitType->name, "Double");
  });
});
