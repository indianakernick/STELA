//
//  html format.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "html format.hpp"

namespace {

constexpr const char doc_start[] =
R"(<!DOCTYPE html>
<html>
<head>
<title>STELA</title>
<meta name="generator" content="STELA by Indi Kernick">
</head>
<body>
)";

constexpr const char doc_end[] =
R"(</body>
</html>
)";

constexpr const char basic_styles[] =
R"(pre {
  margin: 0;
}
)";

constexpr const char basic_doc_styles[] =
R"(body {
  margin: 0;
}
)";

constexpr const char default_styles[] =
R"(pre {
  margin: 0;
  background-color: #000;
  font-family: Consolas, sans-serif;
}
.plain, .oper {
  color: #FFF;
}
.string {
  color: #FF2C38;
}
.character, .number {
  color: #786DFF;
}
.keyword {
  color: #D31895;
}
.type_name {
  color: #00A0FF;
}
.member {
  color: #23FF83;
}
)";

constexpr const char default_doc_styles[] =
R"(body {
  margin: 0;
}
)";

}

std::string stela::htmlFormat(const fmt::Tokens &tokens, const HTMLstyles styles) {
  std::string out;
  out.reserve(tokens.size() * 10);
  if (styles.doc) {
    out += doc_start;
  }
  out += "<style>";
  if (styles.def) {
    if (styles.doc) {
      out += default_doc_styles;
    }
    out += default_styles;
  } else {
    if (styles.doc) {
      out += basic_doc_styles;
    }
    out += basic_styles;
  }
  out += "</style>";
  out += "<pre>\n";
  for (const fmt::Token &tok : tokens) {
    if (const char *str = fmt::tagName(tok.tag)) {
      out += "<span class=\"";
      out += str;
      out += "\">";
      out += tok.text;
      out += "</span>";
    } else if (tok.tag == fmt::Tag::newline) {
      out.append(tok.count, '\n');
    } else if (tok.tag == fmt::Tag::indent) {
      out.append(tok.count * styles.indentSize, ' ');
    }
  }
  out += "</pre>\n";
  if (styles.doc) {
    out += doc_end;
  }
  return out;
}
