//
//  console format.cpp
//  STELA
//
//  Created by Indi Kernick on 14/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "console format.hpp"

#include <Simpleton/Utils/console color.hpp>

namespace {

/* LCOV_EXCL_START */
void write(const stela::ConStyle &style, const std::string_view text) {
  if (style.bold) {
    std::cout << con::bold;
  }
  if (style.faint) {
    std::cout << con::faint;
  }
  if (style.italic) {
    std::cout << con::italic;
  }
  if (style.underline) {
    std::cout << con::underline;
  }
  using Color = stela::ConStyle::Color;
  switch (style.text) {
    case Color::black:
      std::cout << con::text_black; break;
    case Color::red:
      std::cout << con::text_red; break;
    case Color::green:
      std::cout << con::text_green; break;
    case Color::yellow:
      std::cout << con::text_yellow; break;
    case Color::blue:
      std::cout << con::text_blue; break;
    case Color::magenta:
      std::cout << con::text_magenta; break;
    case Color::cyan:
      std::cout << con::text_cyan; break;
    case Color::white:
      std::cout << con::text_white; break;
    case Color::def:
      std::cout << con::text_default; break;
  }
  switch (style.back) {
    case Color::black:
      std::cout << con::back_black; break;
    case Color::red:
      std::cout << con::back_red; break;
    case Color::green:
      std::cout << con::back_green; break;
    case Color::yellow:
      std::cout << con::back_yellow; break;
    case Color::blue:
      std::cout << con::back_blue; break;
    case Color::magenta:
      std::cout << con::back_magenta; break;
    case Color::cyan:
      std::cout << con::back_cyan; break;
    case Color::white:
      std::cout << con::back_white; break;
    case Color::def:
      std::cout << con::back_default; break;
  }
  std::cout << text << con::reset;
}
/* LCOV_EXCL_END */

void repeat(const char c, uint32_t count) {
  while (count--) {
    std::cout.put(c);
  }
}

}

void stela::conFormat(const fmt::Tokens &tokens, const ConStyles &styles) {
  for (const fmt::Token &tok : tokens) {
    if (const auto *style = styles.get(tok.tag)) {
      write(*style, tok.text);
    } else if (tok.tag == fmt::Tag::newline) {
      repeat('\n', tok.count);
    } else if (tok.tag == fmt::Tag::indent) {
      repeat(' ', tok.count * styles.indentSize);
    }
  }
}

void stela::conFormat(const fmt::Tokens &tokens) {
  ConStyles styles;
  styles.indentSize = 2;
  styles.oper.bold = true;
  styles.string.text = ConStyle::Color::red;
  styles.character.text = ConStyle::Color::cyan;
  styles.number.text =  ConStyle::Color::cyan;
  styles.keyword.text = ConStyle::Color::magenta;
  styles.type_name.text = ConStyle::Color::blue;
  styles.member.text = ConStyle::Color::green;
  conFormat(tokens, styles);
}
