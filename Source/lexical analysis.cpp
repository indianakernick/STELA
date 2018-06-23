//
//  lexical analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 17/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lexical analysis.hpp"

#include <cctype>
#include <iostream>
#include <algorithm>
#include <Simpleton/Utils/parse string.hpp>

using namespace stela;

stela::Loc operator+(Utils::LineCol<> lineCol) {
  return {lineCol.line(), lineCol.col()};
}

#define THROW_LEX(MESSAGE) \
{                                                                               \
  log.error(+str.lineCol()) << MESSAGE;                                         \
  log.endError();                                                               \
  throw FatalError{};                                                           \
}

namespace {

constexpr std::string_view keywords[] = {
  "func", "inout", "return",
  "struct", "static", "self", "init",
  "public", "private",
  "enum",
  "let", "var",
  "if", "else",
  "switch", "case", "default",
  "while", "for", "repeat", "in",
  "break", "continue", "fallthrough",
  "typealias"
};
constexpr size_t numKeywords = sizeof(keywords) / sizeof(keywords[0]);

constexpr std::string_view oper[] = {
  "<<=", ">>=", "==", "!=", "<=", ">=", "&&", "||", "--", "++", "->", "+=",
  "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>", "=", "!", "<", ">", "&",
  "|", "^", "~", "{", "}", "(", ")", "[", "]", "+", "-", "*", "/", "%", ".",
  ",", "?", ":", ";"
};
constexpr size_t numOper = sizeof(oper) / sizeof(oper[0]);

void begin(Token &token, const Utils::ParseString &str) {
  token.view = str.beginViewing();
  token.loc.l = str.line();
  token.loc.c = str.col();
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
  return std::isdigit(c);
}

bool continueNumber(const char c) {
  return std::isdigit(c) ||
    c == 'e' || c == 'E' ||
    c == 'x' || c == 'X' ||
    c == 'b' || c == 'B';
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

bool parseString(Token &token, Utils::ParseString &str, Logger &log) {
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

bool parseChar(Token &token, Utils::ParseString &str, Logger &log) {
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

bool parseLineComment(Utils::ParseString &str) {
  if (str.check("//")) {
    str.skipUntil('\n');
    return true;
  } else {
    return false;
  }
}

bool parseMultiComment(Utils::ParseString &str, Logger &log) {
  const Utils::LineCol<> pos = str.lineCol();
  if (str.check("/*")) {
    uint32_t depth = 1;
    while (true) {
      str.skipUntil([] (const char c) {
        return c == '*' || c == '/';
      });
      if (str.check("*/")) {
        --depth;
        if (depth == 0) {
          return true;
        }
      } else if (str.check("/*")) {
        ++depth;
      } else if (str.empty()) {
        log.error(+pos) << "Unterminated multi-line comment";
        log.endError();
        throw FatalError{};
      } else {
        // skipping the '*' or '/' char
        str.advance();
      }
    }
  } else {
    return false;
  }
}

Tokens lexImpl(const std::string_view source, Logger &log) {
  Utils::ParseString str(source);
  std::vector<Token> tokens;
  tokens.reserve(source.size() / 16);
  
  while (true) {
    str.skipWhitespace();
    if (str.empty()) {
      return tokens;
    }
    
    Token token;
    begin(token, str);
    
    if (parseLineComment(str)) continue;
    else if (parseMultiComment(str, log)) continue;
    else if (parseKeyword(token, str)) {}
    else if (parseOper(token, str)) {}
    else if (parseIdent(token, str)) {}
    else if (parseNumber(token, str)) {}
    else if (parseString(token, str, log)) {}
    else if (parseChar(token, str, log)) {}
    else THROW_LEX("Invalid token");
    
    end(token, str);
    tokens.push_back(token);
  }
}

}

Tokens stela::lex(const std::string_view source, Logger &log) try {
  log.cat(stela::LogCat::lexical);
  return lexImpl(source, log);
} catch (stela::FatalError &) {
  throw;
} catch (Utils::ParsingError &e) {
  log.error({e.line(), e.column()}) << e.what();
  log.endError();
  throw FatalError{};
} catch (std::exception &e) {
  throw FatalError{};
}
