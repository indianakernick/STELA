//
//  lexer.cpp
//  Test
//
//  Created by Indi Kernick on 20/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lexer.hpp"

#include "macros.hpp"
#include <STELA/lexer.hpp>

namespace {

[[maybe_unused]]
void printTokens(const std::vector<stela::Token> &tokens) {
  for (const stela::Token &token : tokens) {
    std::cout << token.line << ':' << token.col << "  \t";
    
    switch (token.type) {
      case stela::Token::Type::keyword:
        std::cout << "KEYWORD     ";
        break;
      case stela::Token::Type::identifier:
        std::cout << "IDENTIFIER  ";
        break;
      case stela::Token::Type::number:
        std::cout << "NUMBER      ";
        break;
      case stela::Token::Type::string:
        std::cout << "STRING      ";
        break;
      case stela::Token::Type::character:
        std::cout << "CHARACTER   ";
        break;
      case stela::Token::Type::oper:
        std::cout << "OPERATOR    ";
        break;
      default:
        std::cout << "            ";
    }
    
    std::cout << token.view << '\n';
  }
}

}

#define ASSERT_TYPE(INDEX, TYPE) \
  ASSERT_EQ(tokens[INDEX].type, stela::Token::Type::TYPE)
#define ASSERT_VIEW(INDEX, VIEW) \
  ASSERT_EQ(tokens[INDEX].view, VIEW)

TEST_GROUP(Lexer, {
  TEST(Hello world, {
    const std::vector<stela::Token> tokens = stela::lex(R"(func main(argc: Int) {
      print("Hello world!");
      return 0;
    })");
    
    ASSERT_EQ(tokens.size(), 17);
    
    ASSERT_TYPE(0, keyword);
    ASSERT_TYPE(1, identifier);
    ASSERT_TYPE(2, oper);
    ASSERT_TYPE(3, identifier);
    ASSERT_TYPE(4, oper);
    ASSERT_TYPE(5, identifier);
    ASSERT_TYPE(6, oper);
    ASSERT_TYPE(7, oper);
    ASSERT_TYPE(8, identifier);
    ASSERT_TYPE(9, oper);
    ASSERT_TYPE(10, string);
    ASSERT_TYPE(11, oper);
    ASSERT_TYPE(12, oper);
    ASSERT_TYPE(13, keyword);
    ASSERT_TYPE(14, number);
    ASSERT_TYPE(15, oper);
    ASSERT_TYPE(16, oper);
    
    ASSERT_VIEW(0, "func");
    ASSERT_VIEW(1, "main");
    ASSERT_VIEW(2, "(");
    ASSERT_VIEW(3, "argc");
    ASSERT_VIEW(4, ":");
    ASSERT_VIEW(5, "Int");
    ASSERT_VIEW(6, ")");
    ASSERT_VIEW(7, "{");
    ASSERT_VIEW(8, "print");
    ASSERT_VIEW(9, "(");
    ASSERT_VIEW(10, "\"Hello world!\"");
    ASSERT_VIEW(11, ")");
    ASSERT_VIEW(12, ";");
    ASSERT_VIEW(13, "return");
    ASSERT_VIEW(14, "0");
    ASSERT_VIEW(15, ";");
    ASSERT_VIEW(16, "}");
  });
  
  TEST(Simple strings, {
    const stela::Tokens tokens = stela::lex(
      R"( "string" "string \" with ' quotes" )"
    );
    
    ASSERT_EQ(tokens.size(), 2);
    
    ASSERT_TYPE(0, string);
    ASSERT_TYPE(1, string);
    
    ASSERT_VIEW(0, "\"string\"");
    ASSERT_VIEW(1, "\"string \\\" with ' quotes\"");
  });
  
  TEST(Unterminated string, {
    ASSERT_THROWS(stela::lex("\"unterminated"), stela::LexerError);
  });
  
  TEST(Simple characters, {
    const stela::Tokens tokens = stela::lex(R"( 'c' '\'' )");
    
    ASSERT_EQ(tokens.size(), 2);
    
    ASSERT_TYPE(0, character);
    ASSERT_TYPE(1, character);
    
    ASSERT_VIEW(0, "'c'");
    ASSERT_VIEW(1, "'\\''");
  });
  
  TEST(Unterminated character, {
    ASSERT_THROWS(stela::lex("'u"), stela::LexerError);
  });
})

#undef ASSERT_VIEW
#undef ASSERT_TYPE
