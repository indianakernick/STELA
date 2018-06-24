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

#define GET_NODE(NODE_TYPE, NODE_PTR)                                           \
[&] {                                                                            \
  ASSERT_TRUE(NODE_PTR);                                                        \
  const NODE_TYPE *node = dynamic_cast<const NODE_TYPE *>(NODE_PTR.get());      \
  ASSERT_TRUE(node);                                                            \
  return node;                                                                  \
}()

TEST_GROUP(Syntax, {
  stela::StreamLog log;

  TEST(No tokens, {
    const stela::AST ast = stela::createAST(stela::Tokens{}, log);
    ASSERT_TRUE(ast.topNodes.empty());
  });
  
  TEST(Enum - empty, {
    const char *source = R"(
      enum NoCases {}
    )";
    const stela::AST ast = stela::createAST(source, log);
    ASSERT_EQ(ast.topNodes.size(), 1);
    auto *enumNode = GET_NODE(stela::ast::Enum, ast.topNodes[0]);
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
    const stela::AST ast = stela::createAST(source, log);
    ASSERT_EQ(ast.topNodes.size(), 1);
    auto *enumNode = GET_NODE(stela::ast::Enum, ast.topNodes[0]);
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
    ASSERT_THROWS(stela::createAST(source, log), stela::FatalError);
  });
  
  TEST(Enum - missing comma, {
    const char *source = R"(
      enum ExtraComma {
        first
        second
      }
    )";
    ASSERT_THROWS(stela::createAST(source, log), stela::FatalError);
  });
  
  TEST(Func - empty, {
    const char *source = R"(
      func empty() {}
    )";
    const stela::AST ast = stela::createAST(source, log);
    ASSERT_EQ(ast.topNodes.size(), 1);
    auto *funcNode = GET_NODE(stela::ast::Func, ast.topNodes[0]);
    ASSERT_EQ(funcNode->name, "empty");
    ASSERT_TRUE(funcNode->params.empty());
  });
  
  TEST(Func - one param, {
    const char *source = R"(
      func oneParam(one: Int) -> Void {}
    )";
    const stela::AST ast = stela::createAST(source, log);
    ASSERT_EQ(ast.topNodes.size(), 1);
    auto *funcNode = GET_NODE(stela::ast::Func, ast.topNodes[0]);
    ASSERT_EQ(funcNode->name, "oneParam");
    ASSERT_EQ(funcNode->params.size(), 1);
    
    ASSERT_EQ(funcNode->params[0].name, "one");
    ASSERT_EQ(funcNode->params[0].ref, stela::ast::ParamRef::value);
  });
  
  TEST(Func - two param, {
    const char *source = R"(
      func swap(first: inout Int, second: inout Int) {}
    )";
    const stela::AST ast = stela::createAST(source, log);
    ASSERT_EQ(ast.topNodes.size(), 1);
    auto *funcNode = GET_NODE(stela::ast::Func, ast.topNodes[0]);
    ASSERT_EQ(funcNode->name, "swap");
    ASSERT_EQ(funcNode->params.size(), 2);
    
    ASSERT_EQ(funcNode->params[0].name, "first");
    ASSERT_EQ(funcNode->params[1].name, "second");
    ASSERT_EQ(funcNode->params[0].ref, stela::ast::ParamRef::inout);
    ASSERT_EQ(funcNode->params[0].ref, stela::ast::ParamRef::inout);
  });
});
