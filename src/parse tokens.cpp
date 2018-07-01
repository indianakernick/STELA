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

stela::Context::~Context() {
  assert(stack.stack.size() == index + 1);
  stack.stack.pop_back();
}

void stela::Context::desc(const std::string_view desc) {
  assert(index < stack.stack.size());
  stack.stack[index].desc = desc;
}

void stela::Context::ident(const std::string_view ident) {
  assert(index < stack.stack.size());
  stack.stack[index].ident = ident;
}

stela::Context::Context(ContextStack &stack, const size_t index)
  : stack{stack}, index{index} {}

stela::Context stela::ContextStack::context(const std::string_view desc) {
  const size_t index = stack.size();
  stack.push_back(ContextData{desc, {}});
  return Context{*this, index};
}

std::ostream &stela::operator<<(std::ostream &stream, const ContextStack &stack) {
  if (!stack.stack.empty()) {
    stream << ' ';
  }
  for (auto c = stack.stack.crbegin(); c != stack.stack.crend(); ++c) {
    stream << c->desc;
    if (!c->ident.empty()) {
      stream << " \"" << c->ident << '"';
    }
    if (c != stack.stack.crend() - 1) {
      stream << ", ";
    }
  }
  return stream;
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

stela::Context stela::ParseTokens::context(const std::string_view desc) {
  return ctxStack.context(desc);
}

const stela::ContextStack &stela::ParseTokens::contextStack() const {
  return ctxStack;
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
    log_.ferror(beg->loc) << "Expected " << type << " but found " << *beg
      << ctxStack << endlog;
  }
  const std::string_view view = beg->view;
  ++beg;
  return view;
}

void stela::ParseTokens::expect(const Token::Type type, const std::string_view view) {
  expectToken();
  if (beg->type != type || beg->view != view) {
    log_.ferror(beg->loc) << "Expected " << type << ' ' << view
      << " but found " << *beg << ctxStack << endlog;
  }
  ++beg;
}

std::string_view stela::ParseTokens::expectEither(
  const Token::Type type, const std::string_view a, const std::string_view b
) {
  expectToken();
  if (beg->type != type || (beg->view != a && beg->view != b)) {
    log_.ferror(beg->loc) << "Expected " << type << ' ' << a << " or " << b
      << " but found " << *beg << ctxStack << endlog;
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
    log_.ferror() << "Unexpected end of input" << ctxStack << endlog;
  }
}
