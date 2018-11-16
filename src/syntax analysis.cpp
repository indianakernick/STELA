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

AST stela::createAST(const Tokens &tokens, LogBuf &buf) {
  Log log{buf, LogCat::syntax};
  log.verbose() << "Parsing " << tokens.size() << " tokens" << endlog;

  AST ast;
  ParseTokens tok(tokens, log);
  
  if (tok.checkKeyword("module")) {
    ast.name = tok.expectID();
    tok.expectOp(";");
    tok.extraSemi();
  } else {
    ast.name = "main";
  }
  
  log.status() << "Parsing module \"" << ast.name << "\"" << endlog;
  
  while (!tok.empty()) {
    if (tok.checkKeyword("import")) {
      ast.imports.push_back(tok.expectID());
      tok.expectOp(";");
      tok.extraSemi();
    } else if (ast::DeclPtr node = parseDecl(tok)) {
      ast.global.emplace_back(std::move(node));
      tok.extraSemi();
    } else {
      const Token &token = tok.front();
      log.error(token.loc) << "Unexpected " << token << " in global scope" << endlog;
      log.info(token.loc) << "Only declarations are allowed at global scope" << fatal;
    };
  }
  
  log.verbose() << "Created AST with " << ast.global.size() << " global nodes" << endlog;
  
  return ast;
}

AST stela::createAST(const std::string_view source, LogBuf &buf) {
  return createAST(lex(source, buf), buf);
}
