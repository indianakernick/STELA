//
//  parse tokens.cpp
//  STELA
//
//  Created by Indi Kernick on 24/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse tokens.hpp"

namespace stela {

std::ostream &operator<<(std::ostream &stream, const Token::Type type) {
  switch (type) {
    case Token::Type::keyword:
      return stream << "keyword";
    case Token::Type::identifier:
      return stream << "identifier";
    case Token::Type::number:
      return stream << "number";
    case Token::Type::string:
      return stream << "string";
    case Token::Type::character:
      return stream << "character";
    case Token::Type::oper:
      return stream << "operator";
    default:
      return stream;
  }
}

std::ostream &operator<<(std::ostream &stream, const Token &tok) {
  return stream << tok.type << ' ' << tok.view;
}

}

stela::ParseTokens::ParseTokens(const Tokens &tokens, Log &log)
  : beg{tokens.data()}, end{tokens.data() + tokens.size()}, log_{log} {}

stela::Log &stela::ParseTokens::log() const {
  return log_;
}

bool stela::ParseTokens::empty() const {
  return beg == end;
}

const stela::Token &stela::ParseTokens::front() const {
  return *beg;
}

stela::Loc stela::ParseTokens::lastLoc() const {
  return beg[-1].loc;
}

void stela::ParseTokens::unget() {
  --beg;
}

bool stela::ParseTokens::check(const Token::Type type, const std::string_view view) {
  if (empty() || beg->type != type || beg->view != view) {
    return false;
  } else {
    ++beg;
    return true;
  }
}

bool stela::ParseTokens::checkKeyword(const std::string_view view) {
  return check(Token::Type::keyword, view);
}

bool stela::ParseTokens::checkOp(const std::string_view view) {
  return check(Token::Type::oper, view);
}

std::string_view stela::ParseTokens::expect(const Token::Type type) {
  expectToken();
  if (beg->type != type) {
    log_.ferror(beg->loc) << "Expected " << type << " but found " << *beg << endlog;
  }
  const std::string_view view = beg->view;
  ++beg;
  return view;
}

void stela::ParseTokens::expect(const Token::Type type, const std::string_view view) {
  expectToken();
  if (beg->type != type || beg->view != view) {
    log_.ferror(beg->loc) << "Expected " << type << ' ' << view
      << " but found " << *beg << endlog;
  }
  ++beg;
}

std::string_view stela::ParseTokens::expectEither(const Token::Type type, const std::string_view a, const std::string_view b) {
  expectToken();
  if (beg->type != type || (beg->view != a && beg->view != b)) {
    log_.ferror(beg->loc) << "Expected " << type << ' ' << a << " or " << b
      << " but found " << *beg << endlog;
  }
  const std::string_view view = beg->view;
  ++beg;
  return view;
}

std::string_view stela::ParseTokens::expectID() {
  return expect(Token::Type::identifier);
}

std::string_view stela::ParseTokens::expectOp() {
  return expect(Token::Type::oper);
}

void stela::ParseTokens::expectOp(const std::string_view view) {
  expect(Token::Type::oper, view);
}

std::string_view stela::ParseTokens::expectEitherOp(const std::string_view a, const std::string_view b) {
  return expectEither(Token::Type::oper, a, b);
}

void stela::ParseTokens::expectKeyword(const std::string_view view) {
  expect(Token::Type::keyword, view);
}

void stela::ParseTokens::expectToken() {
  if (empty()) {
    log_.ferror() << "Unexpected end of input" << endlog;
  }
}
