//
//  lexer.cpp
//  Test
//
//  Created by Indi Kernick on 20/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include <gtest/gtest.h>
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

LogSink &log() {
  static StreamSink stream;
  static FilterSink filter{stream, LogPri::info};
  return filter;
}

#define ASSERT_TYPE(INDEX, TYPE) \
  ASSERT_EQ(tokens[INDEX].type, Token::Type::TYPE)
#define ASSERT_VIEW(INDEX, VIEW) \
  ASSERT_EQ(tokens[INDEX].view, VIEW)

TEST(Basic, Empty) {
  const Tokens tokens = tokenize("", log());
  EXPECT_TRUE(tokens.empty());
}

TEST(Basic, Invalid_token) {
  ASSERT_THROW(tokenize("@", log()), FatalError);
}

TEST(Basic, Hello_world) {
  const Tokens tokens = tokenize(R"(func main(argc: sint) {
    print("Hello world!");
    return 0;
  })", log());

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
  ASSERT_VIEW(5, "sint");
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
}

TEST(String, Simple) {
  const Tokens tokens = tokenize(
    R"( "string" "string \" with ' quotes" )",
    log()
  );

  ASSERT_EQ(tokens.size(), 2);

  ASSERT_TYPE(0, string);
  ASSERT_TYPE(1, string);

  ASSERT_VIEW(0, "\"string\"");
  ASSERT_VIEW(1, "\"string \\\" with ' quotes\"");
}

TEST(String, Unterminated) {
  ASSERT_THROW(tokenize("\"unterminated", log()), FatalError);
}

TEST(Character, Simple) {
  const Tokens tokens = tokenize(R"( 'c' '\'' )", log());

  ASSERT_EQ(tokens.size(), 2);

  ASSERT_TYPE(0, character);
  ASSERT_TYPE(1, character);

  ASSERT_VIEW(0, "'c'");
  ASSERT_VIEW(1, "'\\''");
}

TEST(Character, Unterminated) {
  ASSERT_THROW(tokenize("'u", log()), FatalError);
}

TEST(Comment, Single_line) {
  const Tokens tokens = tokenize(R"(
  // this is a comment
  
  // another comment // with // more // slashes //
  )", log());
  ASSERT_TRUE(tokens.empty());
}

TEST(Comment, Multi_line) {
  const Tokens tokens = tokenize(R"(
  /**/
  
  /* cool comment */
  
  /*
  spans
  multiple
  lines
  */
  
  /* just put some / * // ** /// *** in here */
  )", log());
  ASSERT_TRUE(tokens.empty());
}

TEST(Comment, Nested_multi_line) {
  const Tokens tokens = tokenize(R"(
  
  /*
  
  just a regular comment
  
  /* oh but there's a nested bit here */
  
  // is this also nested?
  
  /* oh dear
  
  /* that's a lot of nesting! */
  
  */
  
  I don't think C++ supports nested comments
  
  */
  
  )", log());
  ASSERT_TRUE(tokens.empty());
}

TEST(Comment, Unterminated) {
  ASSERT_THROW(tokenize("/*", log()), FatalError);
  ASSERT_THROW(tokenize("/* /*", log()), FatalError);
  ASSERT_THROW(tokenize("/* /* */", log()), FatalError);
}

TEST(Identifier, WithKeywords) {
  const Tokens tokens = tokenize(
    "staticIdent iforeturn in_that_case continue_to_break let15 vars",
    log()
  );
  ASSERT_EQ(tokens.size(), 6);
  ASSERT_TYPE(0, identifier);
  ASSERT_TYPE(1, identifier);
  ASSERT_TYPE(2, identifier);
  ASSERT_TYPE(3, identifier);
  ASSERT_TYPE(4, identifier);
  ASSERT_TYPE(5, identifier);
}

TEST(Number, Simple) {
  const Tokens tokens = tokenize(
    // ints
    "0 1 2 -0 -1 -2 00 01 02 -00 -01 -02 0x0 0x1 0x2 -0x0 -0x1 -0x2\n"
    // uint64 max
    "4294967295 037777777777 0xFFFFFFFF\n"
    // int64 max
    "2147483647 017777777777 0x7FFFFFFF\n"
    // int64 min
    "-2147483648 -020000000000 -0x80000000\n"
    // floats
    "0.0 0.1 0.2  4.0 4.1 4.9  -0.0 -0.1 -0.2  1e4 1e+4 1e-4  -1e4 -1e+4 -1e-4\n"
    // hex floats
    "0x0.0 0x0.1 0x0.2  0x4.0 0x4.1 0x4.9  -0x0.0 -0x0.1 -0x0.2  0x1p4 0x1p+4 0x1p-4  -0x1p4 -0x1p+4 -0x1p-4\n",
    log()
  );
  ASSERT_EQ(tokens.size(), 57);
  for (const Token &tok : tokens) {
    ASSERT_EQ(tok.type, Token::Type::number);
  }
}

TEST(Number, Byte_range) {
  ASSERT_THROW(tokenize("256b", log()), FatalError);
}

TEST(Number, Char_range) {
  ASSERT_THROW(tokenize("128c", log()), FatalError);
}

/*
@TODO
TEST(Number, Real_range) {
  ASSERT_THROW(tokenize("4.40282e+38r", log()), FatalError);
}
*/

TEST(Number, Sint_range) {
  ASSERT_THROW(tokenize("2147483648s", log()), FatalError);
}

TEST(Number, Uint_range) {
  ASSERT_THROW(tokenize("4294967296u", log()), FatalError);
}

}

#undef ASSERT_VIEW
#undef ASSERT_TYPE
