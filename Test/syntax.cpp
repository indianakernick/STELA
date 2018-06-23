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

TEST_GROUP(Syntax, {
  stela::StreamLog log;

  TEST(No tokens, {
    const stela::AST ast = stela::createAST(stela::Tokens{}, log);
    ASSERT_TRUE(ast.topNodes.empty());
  });
  
  TEST(Empty enum, {
    const char *source = R"(
      enum NoCases {}
    )";
    const stela::AST ast = stela::createAST(source, log);
    ASSERT_EQ(ast.topNodes.size(), 1);
    const stela::ast::NodePtr &node = ast.topNodes[0];
    ASSERT_TRUE(node);
    auto *enumNode = dynamic_cast<const stela::ast::Enum *>(node.get());
    ASSERT_TRUE(enumNode);
    ASSERT_EQ(enumNode->name, "NoCases");
    ASSERT_TRUE(enumNode->cases.empty());
  });
  
  TEST(Direction enum, {
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
    const stela::ast::NodePtr &node = ast.topNodes[0];
    ASSERT_TRUE(node);
    auto *enumNode = dynamic_cast<const stela::ast::Enum *>(node.get());
    ASSERT_TRUE(enumNode);
    ASSERT_EQ(enumNode->name, "Dir");
    ASSERT_EQ(enumNode->cases.size(), 4);
    
    ASSERT_EQ(enumNode->cases[0].name, "up");
    ASSERT_EQ(enumNode->cases[1].name, "right");
    ASSERT_EQ(enumNode->cases[2].name, "down");
    ASSERT_EQ(enumNode->cases[3].name, "left");
  });
});
