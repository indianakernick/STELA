//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

#include <iostream>

using namespace stela;
using namespace stela::ast;

namespace {

using TokIter = Tokens::const_iterator;

bool isKeyword(TokIter &tok, const std::string_view keyword) {
  return (tok->type == Token::Type::keyword && tok->view == keyword);
}

// expect another token
void expectNext(TokIter &tok, const TokIter end, Log &log) {
  const TokIter prev = tok++;
  if (tok == end) {
    log.error(prev->loc) << "Unexpected end of input";
    log.endError();
    throw FatalError{};
  }
}

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

void expectType(TokIter &tok, const Token::Type type, Log &log) {
  if (tok->type != type) {
    log.error(tok->loc) << "Expected " << type << " but found " << tok->type;
    log.endError();
    throw FatalError{};
  }
}

NodePtr parseFunc(TokIter &tok, const TokIter end) {
  return nullptr;
}

NodePtr parseEnum(TokIter &tok, const TokIter end, Log &log) {
  if (!isKeyword(tok, "enum")) {
    return nullptr;
  }
  expectNext(tok, end, log);
  auto enumNode = std::make_unique<ast::Enum>();
  expectType(tok, Token::Type::identifier, log);
  
  return enumNode;
}

NodePtr parseStruct(TokIter &tok, const TokIter end) {
  return nullptr;
}

NodePtr parseTypealias(TokIter &tok, const TokIter end) {
  return nullptr;
}

NodePtr parseLet(TokIter &tok, const TokIter end) {
  return nullptr;
}

AST createASTimpl(const Tokens &tokens, Log &log) {
  AST ast;
  
  const TokIter end = tokens.cend();
  for (TokIter tok = tokens.cbegin(); tok != end; ++tok) {
    ast::NodePtr node;
    if ((node = parseFunc(tok, end))) {}
    else if ((node = parseEnum(tok, end, log))) {}
    else if ((node = parseStruct(tok, end))) {}
    else if ((node = parseTypealias(tok, end))) {}
    else if ((node = parseLet(tok, end))) {}
    else {
      log.error(tok->loc) << "Unexpected token in global scope";
      log.endError();
      log.info(tok->loc) << "Only functions, type declarations and constants are allowed at global scope";
      log.endInfo();
      throw FatalError{};
    };
    ast.topNodes.emplace_back(std::move(node));
  }
  
  return {};
}

}

AST stela::createAST(const Tokens &tokens, Log &log) try {
  log.cat(stela::LogCat::syntax);
  return createASTimpl(tokens, log);
} catch (FatalError &) {
  throw;
} catch (std::exception &e) {
  log.error() << e.what();
  log.endError();
  throw;
}
