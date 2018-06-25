//
//  lexer.cpp
//  Test
//
//  Created by Indi Kernick on 20/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lexer.hpp"

#include "macros.hpp"
#include <STELA/lexical analysis.hpp>

using namespace stela;

namespace {

[[maybe_unused]]
void printTokens(const Tokens &tokens) {
  for (const Token &token : tokens) {
    std::cout << token.loc.l << ':' << token.loc.c << "  \t";
    
    switch (token.type) {
      case Token::Type::keyword:
        std::cout << "KEYWORD     ";
        break;
      case Token::Type::identifier:
        std::cout << "IDENTIFIER  ";
        break;
      case Token::Type::number:
        std::cout << "NUMBER      ";
        break;
      case Token::Type::string:
        std::cout << "STRING      ";
        break;
      case Token::Type::character:
        std::cout << "CHARACTER   ";
        break;
      case Token::Type::oper:
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
  ASSERT_EQ(tokens[INDEX].type, Token::Type::TYPE)
#define ASSERT_VIEW(INDEX, VIEW) \
  ASSERT_EQ(tokens[INDEX].view, VIEW)

TEST_GROUP(Lexer, {
  StreamLog log;

  TEST(Hello world, {
    const std::vector<Token> tokens = lex(R"(func main(argc: Int) {
      print("Hello world!");
      return 0;
    })", log);
    
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
    const Tokens tokens = lex(
      R"( "string" "string \" with ' quotes" )",
      log
    );
    
    ASSERT_EQ(tokens.size(), 2);
    
    ASSERT_TYPE(0, string);
    ASSERT_TYPE(1, string);
    
    ASSERT_VIEW(0, "\"string\"");
    ASSERT_VIEW(1, "\"string \\\" with ' quotes\"");
  });
  
  TEST(Unterminated string, {
    ASSERT_THROWS(lex("\"unterminated", log), FatalError);
  });
  
  TEST(Simple characters, {
    const Tokens tokens = lex(R"( 'c' '\'' )", log);
    
    ASSERT_EQ(tokens.size(), 2);
    
    ASSERT_TYPE(0, character);
    ASSERT_TYPE(1, character);
    
    ASSERT_VIEW(0, "'c'");
    ASSERT_VIEW(1, "'\\''");
  });
  
  TEST(Unterminated character, {
    ASSERT_THROWS(lex("'u", log), FatalError);
  });
  
  TEST(Single line comment, {
    const Tokens tokens = lex(R"(
    // this is a comment
    
    // another comment // with // more // slashes //
    )", log);
    ASSERT_TRUE(tokens.empty());
  });
  
  TEST(Multi line comment, {
    const Tokens tokens = lex(R"(
    /**/
    
    /* cool comment */
    
    /*
    spans
    multiple
    lines
    */
    
    /* just put some / * // ** /// *** in here */
    )", log);
    ASSERT_TRUE(tokens.empty());
  });
  
  TEST(Nested multi line comment, {
    const Tokens tokens = lex(R"(
    
    /*
    
    just a regular comment
    
    /* oh but there's a nested bit here */
    
    // is this also nested?
    
    /* oh dear
    
    /* that's a lot of nesting! */
    
    */
    
    I don't think C++ supports nested comments
    
    */
    
    )", log);
    ASSERT_TRUE(tokens.empty());
  });
  
  TEST(Unterminated multi line comment, {
    ASSERT_THROWS(lex("/*", log), FatalError);
    ASSERT_THROWS(lex("/* /*", log), FatalError);
    ASSERT_THROWS(lex("/* /* */", log), FatalError);
  });
  
  TEST(Keywords in identifiers, {
    const Tokens tokens = lex(
      "staticIdent iforeturn in_that_case continue_to_fallthrough let15 vars",
      log
    );
    ASSERT_EQ(tokens.size(), 6);
    ASSERT_TYPE(0, identifier);
    ASSERT_TYPE(1, identifier);
    ASSERT_TYPE(2, identifier);
    ASSERT_TYPE(3, identifier);
    ASSERT_TYPE(4, identifier);
    ASSERT_TYPE(5, identifier);
  });
  
  TEST(Valid number literals, {
  // 0; 1; 2; -0; -1; -2; 00; 01; 02; -00; -01; -02; 0x0; 0x1; 0x2; -0x0; 0x1; 0x2;
    //uint64_t max oct dec hex
    //int64_t max
    //int64_t min
    const Tokens tokens = lex(
      // ints
      "0 1 2 -0 -1 -2 00 01 02 -00 -01 -02 0x0 0x1 0x2 -0x0 -0x1 -0x2\n"
      // uint64 max
      "18446744073709551615 01777777777777777777777 0xFFFFFFFFFFFFFFFF\n"
      // int64 max
      "9223372036854775807 0777777777777777777777 0x7FFFFFFFFFFFFFFF\n"
      // int64 min
      "-9223372036854775808 -01000000000000000000000 -0x8000000000000000\n"
      // floats
      "0.0 0.1 0.2  4.0 4.1 4.9  -0.0 -0.1 -0.2  1e4 1e+4 1e-4  -1e4 -1e+4 -1e-4\n"
      // hex floats
      "0x0.0 0x0.1 0x0.2  0x4.0 0x4.1 0x4.9  -0x0.0 -0x0.1 -0x0.2  0x1p4 0x1p+4 0x1p-4  -0x1p4 -0x1p+4 -0x1p-4\n"
      ,
      log
    );
    ASSERT_EQ(tokens.size(), 57);
    for (const Token &tok : tokens) {
      ASSERT_EQ(tok.type, Token::Type::number);
    }
  });
})

#undef ASSERT_VIEW
#undef ASSERT_TYPE
