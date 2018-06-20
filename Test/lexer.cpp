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

void printTokens(const std::vector<stela::Token> &tokens) {
  for (const stela::Token &token : tokens) {
    std::cout << token.line << ':' << token.col << "  \t";
    
    switch (token.type) {
      case stela::Token::Type::KEYWORD:
        std::cout << "KEYWORD     ";
        break;
      case stela::Token::Type::IDENTIFIER:
        std::cout << "IDENTIFIER  ";
        break;
      case stela::Token::Type::NUMBER:
        std::cout << "NUMBER      ";
        break;
      case stela::Token::Type::STRING:
        std::cout << "STRING      ";
        break;
      case stela::Token::Type::CHARACTER:
        std::cout << "CHARACTER   ";
        break;
      case stela::Token::Type::OPERATOR:
        std::cout << "OPERATOR    ";
        break;
      default:
        std::cout << "            ";
    }
    
    std::cout << token.view << '\n';
  }
}

const char *helloWorld = R"(func main(argc: Int) {
  print("Hello world!");
  return 0;
})";

}

void testLexer() {
  #define TOK(NAME, TYPE) stela::Token{NAME, 0, 0, stela::Token::Type::TYPE}
  
  TEST(Lexer_HelloWorld, {
  
    const std::vector<stela::Token> should = {
      TOK("func", KEYWORD), TOK("main", IDENTIFIER), TOK("(", OPERATOR),
      TOK("argc", IDENTIFIER), TOK(":", OPERATOR), TOK("Int", IDENTIFIER),
      TOK(")", OPERATOR), TOK("{", OPERATOR), TOK("print", IDENTIFIER),
      TOK("(", OPERATOR), TOK("\"Hello world!\"", STRING), TOK(")", OPERATOR),
      TOK(";", OPERATOR), TOK("return", KEYWORD), TOK("0", NUMBER),
      TOK(";", OPERATOR), TOK("}", OPERATOR)
    };
  
    const std::vector<stela::Token> actual = stela::lex(helloWorld);
    
    ASSERT_EQ(actual.size(), should.size());
    
    for (size_t i = 0; i != should.size(); ++i) {
      std::cout << i << '\n';
      ASSERT_EQ(actual[i].type, should[i].type);
      ASSERT_EQ(actual[i].view, should[i].view);
    }
  });
  
  #undef TOK
}
