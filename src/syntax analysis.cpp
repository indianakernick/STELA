//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

#include "parse decl.hpp"
#include "log output.hpp"
#include "lexical analysis.hpp"

using namespace stela;

namespace {

AST createASTimpl(const Tokens &tokens, Log &log) {
  log.verbose() << "Parsing " << tokens.size() << " tokens" << endlog;

  AST ast;
  ParseTokens tok(tokens, log);
  
  while (!tok.empty()) {
    if (tok.checkOp(";")) {
      log.warn(tok.lastLoc()) << "Unnecessary ;" << endlog;
      continue;
    }
  
    if (ast::DeclPtr node = parseDecl(tok)) {
      ast.global.emplace_back(std::move(node));
    } else {
      const Token &token = tok.front();
      log.info(token.loc) << "Only declarations are allowed at global scope" << endlog;
      log.ferror(token.loc) << "Unexpected " << token << " in global scope" << endlog;
    };
  }
  
  log.verbose() << "Created AST with " << ast.global.size() << " global nodes" << endlog;
  
  return ast;
}

}

AST stela::createAST(const Tokens &tokens, LogBuf &buf) {
  Log log{buf, LogCat::syntax};
  try {
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
