//
//  log.cpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log.hpp"

#include <cassert>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <Simpleton/Utils/console color.hpp>

using namespace stela;

LogBuf::~LogBuf() = default;

std::ostream &stela::operator<<(std::ostream &stream, const LogCat cat) {
  // Categories are adjectives
  switch (cat) {
    case LogCat::lexical:
      return stream << "Lexical";
    case LogCat::syntax:
      return stream << "Syntactic";
    case LogCat::semantic:
      return stream << "Semantic";
    case LogCat::generate:
      return stream << "Generative";
    /* LCOV_EXCL_START */
    default:
      assert(false);
      return stream;
    /* LCOV_EXCL_END */
  }
}

std::ostream &stela::operator<<(std::ostream &stream, const LogPri pri) {
  // Priorities are nouns
  switch (pri) {
    case LogPri::verbose:
      return stream << "verbosity";
    case LogPri::status:
      return stream << "status";
    case LogPri::info:
      return stream << "info";
    case LogPri::warning:
      return stream << "warning";
    case LogPri::error:
      return stream << "error";
    /* LCOV_EXCL_START */
    case LogPri::nothing:
    default:
      assert(false);
      return stream;
    /* LCOV_EXCL_END */
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
};

}

std::streambuf *stela::silentBuf() {
  static SilentBuf buf;
  return &buf;
}

void stela::LogBuf::pri(const LogPri newPri) {
  priority = newPri;
}

void stela::LogBuf::beginLog(const LogCat cat, const LogPri pri, const LogMod mod, const Loc loc) {
  if (canLog(pri)) {
    begin(cat, pri, mod, loc);
  }
}

void stela::LogBuf::beginLog(const LogCat cat, const LogPri pri, const LogMod mod) {
  if (canLog(pri)) {
    begin(cat, pri, mod);
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

namespace {

int ceilToMultiple(const int factor, const int num) {
  assert(factor > 0);
  return (num + factor - 1) / factor * factor;
}

int locationWidth(const size_t modWidth) {
  return ceilToMultiple(8, static_cast<int>(modWidth) + 6);
}

void writeMod(std::ostream &stream, const LogMod mod) {
  if (!mod.empty()) {
    stream << std::left << std::setw(locationWidth(mod.size())) << mod;
  }
}

void writeModLoc(std::ostream &stream, const LogMod mod, const Loc loc) {
  std::ostringstream str;
  if (!mod.empty()) {
    str << mod << ':';
  }
  str << loc;
  stream << std::left << std::setw(locationWidth(mod.size())) << str.str();
}

void priorityColor(const LogPri pri) {
  if (pri == LogPri::verbose) {
    std::cerr << con::text_magenta;
  } else if (pri == LogPri::status) {
    std::cerr << con::text_green;
  } else if (pri == LogPri::info) {
    std::cerr << con::text_blue;
  } else if (pri == LogPri::warning) {
    std::cerr << con::text_yellow;
  } else if (pri == LogPri::error) {
    std::cerr << con::text_red;
  } else {
    /* LCOV_EXCL_START */
    assert(false);
    /* LCOV_EXCL_END */
  }
}

}

void stela::StreamLog::begin(const LogCat cat, const LogPri pri, const LogMod mod, const Loc loc) {
  std::ostream stream{buf};
  writeModLoc(stream, mod, loc);
  stream << cat << ' ' << pri << ": ";
}

void stela::StreamLog::begin(const LogCat cat, const LogPri pri, const LogMod mod) {
  std::ostream stream{buf};
  writeMod(stream, mod);
  stream << cat << ' ' << pri << ": ";
}

void stela::StreamLog::end(LogCat, LogPri) {
  std::ostream stream{buf};
  stream << std::endl;
}

std::streambuf *stela::ColorLog::getBuf(LogCat, LogPri) {
  return std::cerr.rdbuf();
}

void stela::ColorLog::begin(const LogCat cat, const LogPri pri, const LogMod mod, const Loc loc) {
  std::cerr << con::bold;
  writeModLoc(std::cerr, mod, loc);
  std::cerr << con::no_bold_faint;
  priorityColor(pri);
  std::cerr << cat << ' ' << pri;
  std::cerr << con::text_default << ": " << con::italic;
}

void stela::ColorLog::begin(const LogCat cat, const LogPri pri, const LogMod mod) {
  std::cerr << con::bold;
  writeMod(std::cerr, mod);
  std::cerr << con::no_bold_faint;
  priorityColor(pri);
  std::cerr << cat << ' ' << pri;
  std::cerr << con::text_default << ": " << con::italic;
}

void stela::ColorLog::end(LogCat, LogPri) {
  std::cerr << con::no_italic << std::endl;
}

std::streambuf *stela::NoLog::getBuf(LogCat, LogPri) {
  return silentBuf();
}
void stela::NoLog::begin(LogCat, LogPri, LogMod, Loc) {}
void stela::NoLog::begin(LogCat, LogPri, LogMod) {}
void stela::NoLog::end(LogCat, LogPri) {}
