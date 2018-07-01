//
//  lexical analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 17/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lexical analysis.hpp"

#include <cctype>
#include <algorithm>
#include "log output.hpp"
#include "number literal.hpp"
#include <Simpleton/Utils/parse string.hpp>

using namespace stela;

stela::Loc operator+(Utils::LineCol<> lineCol) {
  return {lineCol.line(), lineCol.col()};
}

namespace {

constexpr std::string_view keywords[] = {
  "func", "inout", "return", "in",
  "struct", "static", "self", "init", "make",
  "public", "private",
  "enum", "true", "false",
  "let", "var",
  "if", "else",
  "switch", "case", "default",
  "while", "for", "repeat",
  "break", "continue", "fallthrough",
  "typealias",
  
  "Void",
  "Int",
  "Char",
  "Bool",
  "Float",
  "Double",
  "String"
};
constexpr size_t numKeywords = sizeof(keywords) / sizeof(keywords[0]);

constexpr std::string_view oper[] = {
  "<<=", ">>=", "==", "!=", "<=", ">=", "&&", "||", "--", "++", "->", "+=",
  "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>", "[{", "}]", "=", "!", "<", ">", "&",
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

bool parseKeyword(Token &token, Utils::ParseString &str) {
  if (str.tryParseEnum(keywords, numKeywords, std::not_fn(continueIdent)) != numKeywords) {
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

bool parseNumber(Token &token, Utils::ParseString &str, Log &log) {
  const size_t numberChars = validNumberLiteral(str.view(), log);
  if (numberChars == 0) {
    return false;
  } else {
    str.advance(numberChars);
    token.type = Token::Type::number;
    return true;
  }
}

bool parseString(Token &token, Utils::ParseString &str, Log &log) {
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
        log.ferror(+str.lineCol()) << "Unterminated string literal" << endlog;
      }
    }
  } else {
    return false;
  }
}

bool parseChar(Token &token, Utils::ParseString &str, Log &log) {
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
        log.ferror(+str.lineCol()) << "Unterminated character literal" << endlog;
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

bool parseMultiComment(Utils::ParseString &str, Log &log) {
  const Loc loc = +str.lineCol();
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
        log.ferror(loc) << "Unterminated multi-line comment" << endlog;
      } else {
        // skipping the '*' or '/' char
        str.advance();
      }
    }
  } else {
    return false;
  }
}

Tokens lexImpl(const std::string_view source, Log &log) {
  log.verbose() << "Parsing " << source.size() << " characters" << endlog;
  
  Utils::ParseString str(source);
  std::vector<Token> tokens;
  tokens.reserve(source.size() / 16);
  
  while (true) {
    str.skipWhitespace();
    if (str.empty()) {
      log.verbose() << "Created " << tokens.size() << " tokens" << endlog;
      return tokens;
    }
    
    Token token;
    begin(token, str);
    
    if (parseLineComment(str)) continue;
    else if (parseMultiComment(str, log)) continue;
    else if (parseKeyword(token, str)) {}
    else if (parseNumber(token, str, log)) {}
    else if (parseOper(token, str)) {}
    else if (parseIdent(token, str)) {}
    else if (parseString(token, str, log)) {}
    else if (parseChar(token, str, log)) {}
    else log.ferror(+str.lineCol()) << "Invalid token" << endlog;
    
    end(token, str);
    tokens.push_back(token);
  }
}

}

Tokens stela::lex(const std::string_view source, LogBuf &buf) {
  Log log{buf, LogCat::lexical};
  try {
    return lexImpl(source, log);
  } catch (stela::FatalError &) {
    throw;
  } catch (Utils::ParsingError &e) {
    log.ferror({e.line(), e.column()}) << e.what() << endlog;
    return {};
  } catch (std::exception &e) {
    log.error() << e.what() << endlog;
    throw;
  }
}
