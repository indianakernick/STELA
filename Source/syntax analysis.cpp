//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

#include "log output.hpp"
#include "parse tokens.hpp"
#include "lexical analysis.hpp"

using namespace stela;
using namespace stela::ast;

namespace {

NodePtr parseFunc(ParseTokens &) {
  return nullptr;
}

NodePtr parseExpr(ParseTokens &) {
  return nullptr;
}

NodePtr parseEnum(ParseTokens &tok) {
  if (!tok.checkKeyword("enum")) {
    return nullptr;
  }
  auto enumNode = std::make_unique<ast::Enum>();
  enumNode->name = tok.expectID();
  tok.expectOp("{");
  
  if (!tok.checkOp("}")) {
    do {
      EnumCase &ecase = enumNode->cases.emplace_back();
      ecase.name = tok.expectID();
      if (tok.checkOp("=")) {
        ecase.value = parseExpr(tok);
      }
    } while (tok.expectEitherOp("}", ",") == ",");
  }
  
  if (tok.checkOp(";")) {
    tok.log().warn(tok.lastLoc()) << "Unnecessary ;" << endlog;
  }
  
  return enumNode;
}

NodePtr parseStruct(ParseTokens &) {
  return nullptr;
}

NodePtr parseTypealias(ParseTokens &) {
  return nullptr;
}

NodePtr parseLet(ParseTokens &) {
  return nullptr;
}

AST createASTimpl(const Tokens &tokens, Log &log) {
  AST ast;
  ParseTokens tok(tokens, log);
  
  while (!tok.empty()) {
    ast::NodePtr node;
    if ((node = parseFunc(tok))) {}
    else if ((node = parseEnum(tok))) {}
    else if ((node = parseStruct(tok))) {}
    else if ((node = parseTypealias(tok))) {}
    else if ((node = parseLet(tok))) {}
    else {
      const Token &token = tok.front();
      log.info(token.loc) << "Only functions, type declarations and constants "
        "are allowed at global scope" << endlog;
      log.ferror(token.loc) << "Unexpected token in global scope" << endlog;
    };
    ast.topNodes.emplace_back(std::move(node));
  }
  
  return ast;
}

}

AST stela::createAST(const Tokens &tokens, LogBuf &buf) {
  Log log{buf};
  try {
    log.cat(stela::LogCat::syntax);
    return createASTimpl(tokens, log);
  } catch (FatalError &) {
    throw;
  } catch (std::exception &e) {
    log.error() << e.what() << endlog;
    throw;
  }
}

AST stela::createAST(const std::string_view source, LogBuf &buf) {
  return createAST(lex(source, buf), buf);
}
