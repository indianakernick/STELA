//
//  lexer.cpp
//  STELA
//
//  Created by Indi Kernick on 17/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lexer.hpp"

#include <cctype>
#include <algorithm>
#include <Simpleton/Utils/parse string.hpp>

using namespace stela;

stela::LexerError::LexerError(
  const uint32_t line, const uint32_t col, const char *msg
) : std::runtime_error(
  "Lexer Error - "
  + std::to_string(line)
  + ':'
  + std::to_string(col)
  + " - "
  + msg
), line_(line), col_(col) {}

uint32_t stela::LexerError::line() const {
  return line_;
}

uint32_t stela::LexerError::col() const {
  return col_;
}

#define THROW_LEX(MESSAGE)                                                      \
  throw stela::LexerError{str.line(), str.col(), MESSAGE}

namespace {

constexpr std::string_view keywords[] = {
  "func", "return",
  "class", "public", "private", "protected", "static", "self", "super", "override"
  "struct",
  "enum",
  "let", "var",
  "if", "else",
  "switch", "case", "default",
  "while", "for", "break", "continue"
  "init", "deinit",
  "is", "as", "nil",
  "true", "false",
  
};
constexpr size_t numKeywords = sizeof(keywords) / sizeof(keywords[0]);

constexpr std::string_view oper[] = {
  "==", "!=", "<=", ">=", "&&", "||", "->", "=", "!", "<", ">", "&", "|", "{",
  "}", "(", ")", "[", "]", "+", "-", "*", "/", "%", "~", ".", ",", ":", ";", "?"
};
constexpr size_t numOper = sizeof(oper) / sizeof(oper[0]);

void begin(Token &token, const Utils::ParseString &str) {
  token.view = str.beginViewing();
  const auto lineCol = str.lineCol();
  token.line = lineCol.line();
  token.col = lineCol.col();
}

void end(Token &token, const Utils::ParseString &str) {
  str.endViewing(token.view);
}

bool startIdent(const char c) {
  return std::isalpha(c);
}

bool continueIdent(const char c) {
  return std::isalnum(c) || c == '_';
}

bool startNumber(const char c) {
  return std::isdigit(c) || c == '.' || c == '-' || c == '+';
}

bool continueNumber(const char c) {
  return startNumber(c) || c == 'e' || c == 'E' || c == 'x' || c == 'X';
}

bool parseKeyword(Token &token, Utils::ParseString &str) {
  if (str.tryParseEnum(keywords, numKeywords) != numKeywords) {
    token.type = Token::Type::keyword;
    return true;
  } else {
    return false;
  }
}

bool parseIdent(Token &token, Utils::ParseString &str) {
  if (str.check(startIdent)) {
    str.skip(continueIdent);
    token.type = Token::Type::identifier;
    return true;
  } else {
    return false;
  }
}

bool parseNumber(Token &token, Utils::ParseString &str) {
  if (str.check(startNumber)) {
    str.skip(continueNumber);
    token.type = stela::Token::Type::number;
    return true;
  } else {
    return false;
  }
}

bool parseString(Token &token, Utils::ParseString &str) {
  if (str.check('"')) {
    while (true) {
      str.skipUntil([] (const char c) {
        return c == '\n' || c == '\\' || c == '"';
      });
      if (str.check('"')) {
        token.type = Token::Type::string;
        return true;
      } else if (str.check('\\')) {
        str.advance();
      } else {
        THROW_LEX("Unterminated string literal");
      }
    }
  } else {
    return false;
  }
}

bool parseChar(Token &token, Utils::ParseString &str) {
  if (str.check('\'')) {
    while (true) {
      str.skipUntil([] (const char c) {
        return c == '\n' || c == '\\' || c == '\'';
      });
      if (str.check('\'')) {
        token.type = Token::Type::character;
        return true;
      } else if (str.check('\\')) {
        str.advance();
      } else {
        THROW_LEX("Unterminated character literal");
      }
    }
  } else {
    return false;
  }
}

bool parseOper(Token &token, Utils::ParseString &str) {
  if (str.tryParseEnum(oper, numOper) != numOper) {
    token.type = Token::Type::oper;
    return true;
  } else {
    return false;
  }
}

Tokens lexImpl(const std::string_view source) {
  Utils::ParseString str(source);
  std::vector<Token> tokens;
  tokens.reserve(source.size() / 16);
  
  while (true) {
    str.skipWhitespace();
    if (str.empty()) {
      return tokens;
    }
    
    Token &token = tokens.emplace_back();
    begin(token, str);
    
    if (parseKeyword(token, str)) {}
    else if (parseIdent(token, str)) {}
    else if (parseNumber(token, str)) {}
    else if (parseString(token, str)) {}
    else if (parseChar(token, str)) {}
    else if (parseOper(token, str)) {}
    else THROW_LEX("Invalid token");
    
    end(token, str);
  }
}

}

Tokens stela::lex(const std::string_view source) try {
  return lexImpl(source);
} catch (stela::LexerError &) {
  throw;
} catch (Utils::ParsingError &e) {
  throw stela::LexerError(e.line(), e.column(), e.what());
} catch (std::exception &e) {
  throw stela::LexerError(0, 0, e.what());
} catch (...) {
  throw stela::LexerError(0, 0, "Unknown error");
}
