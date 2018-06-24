//
//  log.cpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log.hpp"

#include <iomanip>
#include <sstream>
#include <iostream>
#define TERM_STREAM stderr
#include <Simpleton/Utils/terminal color.hpp>

namespace Term = Utils::Term;

std::ostream &stela::operator<<(std::ostream &stream, const LogCat cat) {
  switch (cat) {
    case LogCat::lexical:
      return stream << "Lexical";
    case LogCat::syntax:
      return stream << "Syntax";
    case LogCat::semantic:
      return stream << "Semantic";
    default:
      return stream;
  }
}

std::ostream &stela::operator<<(std::ostream &stream, const LogPri pri) {
  switch (pri) {
    case LogPri::info:
      return stream << "info";
    case LogPri::warning:
      return stream << "warning";
    case LogPri::error:
    case LogPri::fatal_error:
      return stream << "error";
    default:
      return stream;
  }
}

std::ostream &stela::operator<<(std::ostream &stream, const Loc loc) {
  return stream << loc.l << ':' << loc.c;
}

const char *stela::FatalError::what() const noexcept {
  return "Fatal error";
}

namespace {

class SilentBuf : public std::streambuf {
public:
  std::streamsize xsputn(const char_type *, const std::streamsize n) override {
    return n;
  }
  int_type overflow(const int_type c) override {
    return c;
  }
};

}

std::streambuf *stela::silentBuf() {
  static SilentBuf buf;
  return &buf;
}


void stela::LogBuf::pri(const LogPri newPri) {
  priority = newPri;
}

void stela::LogBuf::beginLog(const LogCat cat, const LogPri pri, const Loc loc) {
  if (canLog(pri)) {
    begin(cat, pri, loc);
  }
}

void stela::LogBuf::beginLog(const LogCat cat, const LogPri pri) {
  if (canLog(pri)) {
    begin(cat, pri);
  }
}

void stela::LogBuf::endLog(const LogCat cat, const LogPri pri) {
  if (canLog(pri)) {
    end(cat, pri);
  }
}

std::streambuf *stela::LogBuf::getStreambuf(const LogCat cat, const LogPri pri) {
  if (canLog(pri)) {
    return getBuf(cat, pri);
  } else {
    return silentBuf();
  }
}

bool stela::LogBuf::canLog(const LogPri pri) const {
  return static_cast<uint8_t>(pri) >= static_cast<uint8_t>(priority);
}

stela::StreamLog::StreamLog()
  : StreamLog{std::cerr} {}

stela::StreamLog::StreamLog(const std::ostream &stream)
  : buf{stream.rdbuf()} {}

std::streambuf *stela::StreamLog::getBuf(LogCat, LogPri) {
  return buf;
}

void stela::StreamLog::begin(const LogCat cat, const LogPri pri, const Loc loc) {
  std::ostringstream str;
  str << loc;
  std::ostream stream{buf};
  stream << std::left << std::setw(8) << str.str();
  begin(cat, pri);
}

void stela::StreamLog::begin(const LogCat cat, const LogPri pri) {
  std::ostream stream{buf};
  stream << cat << ' ' << pri << ": ";
}

void stela::StreamLog::end(LogCat, LogPri) {
  std::ostream stream{buf};
  stream << '\n';
}

std::streambuf *stela::ColorLog::getBuf(LogCat, LogPri) {
  return std::cerr.rdbuf();
}

void stela::ColorLog::begin(const LogCat cat, const LogPri pri, const Loc loc) {
  std::ostringstream str;
  str << loc;
  Term::intensity(Term::Intensity::BOLD);
  std::cerr << std::left << std::setw(8) << str.str();
  Term::intensity(Term::Intensity::NORMAL);
  begin(cat, pri);
}

void stela::ColorLog::begin(const LogCat cat, const LogPri pri) {
  if (pri == LogPri::info) {
    Term::textColor(Term::Color::BLUE);
  } else if (pri == LogPri::warning) {
    Term::textColor(Term::Color::YELLOW);
  } else if (pri == LogPri::error || pri == LogPri::fatal_error) {
    Term::textColor(Term::Color::RED);
  }
  std::cerr << cat << ' ' << pri;
  Term::defaultTextColor();
  std::cerr << ": ";
  Term::italic(true);
}

void stela::ColorLog::end(LogCat, LogPri) {
  Term::italic(false);
  std::cerr << '\n';
}

std::streambuf *stela::NoLog::getBuf(LogCat, LogPri) {
  return silentBuf();
}
void stela::NoLog::begin(LogCat, LogPri, Loc) {}
void stela::NoLog::begin(LogCat, LogPri) {}
void stela::NoLog::end(LogCat, LogPri) {}
