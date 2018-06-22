//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

using namespace stela;
using namespace stela::ast;

stela::SyntaxError::SyntaxError(const Loc loc, const char *msg)
  : Error{"Syntax Error", loc, msg} {}

#define THROW_SYNTAX(MSG)                                                       \
  throw stela::SyntaxError(tok->loc, MSG)

namespace {

using TokIter = Tokens::const_iterator;

bool isKeyword(TokIter &tok, const std::string_view keyword) {
  return (tok->type == Token::Type::keyword && tok->view == keyword);
}

// expect another token
void expectNext(TokIter &tok, const TokIter end) {
  const TokIter prev = tok++;
  if (tok == end) {
    throw stela::SyntaxError(prev->loc, "Unexpected end of input");
  }
}

void expectType(TokIter &tok, const Token::Type type) {
  if (tok->type != type) {
    throw stela::SyntaxError(tok->loc, "Expected token ");
  }
}

NodePtr parseFunc(TokIter &tok, const TokIter end) {
  return nullptr;
}

NodePtr parseEnum(TokIter &tok, const TokIter end) {
  if (!isKeyword(tok, "enum")) {
    return nullptr;
  }
  expectNext(tok, end);
  const auto enumNode = std::make_unique<ast::Enum>();
  expectType(tok, Token::Type::identifier);
  
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

AST createASTimpl(const Tokens &tokens) {
  AST ast;
  
  const TokIter end = tokens.cend();
  for (TokIter tok = tokens.cbegin(); tok != end; ++tok) {
    ast::NodePtr node;
    if ((node = parseFunc(tok, end))) {}
    else if ((node = parseEnum(tok, end))) {}
    else if ((node = parseStruct(tok, end))) {}
    else if ((node = parseTypealias(tok, end))) {}
    else if ((node = parseLet(tok, end))) {}
    else THROW_SYNTAX(
      "Unexpected token in global scope."
      "Only functions, type declarations and constants are allowed at global scope"
    );
    ast.topNodes.push_back(node);
  }
  
  return {};
}

}

AST stela::createAST(const Tokens &tokens) try {
  return createASTimpl(tokens);
} catch (SyntaxError &) {
  throw;
} catch (std::exception &e) {
  throw SyntaxError({0, 0}, e.what());
}
