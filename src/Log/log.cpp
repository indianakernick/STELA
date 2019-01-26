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
#include "Utils/unreachable.hpp"
#include "Utils/console color.hpp"

using namespace stela;

LogSink::~LogSink() = default;

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
  }
  UNREACHABLE();
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
    default: UNREACHABLE();
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

namespace {

int ceilToMultiple(const int factor, const int num) {
  assert(factor > 0);
  return (num + factor - 1) / factor * factor;
}

int locationWidth(const size_t modWidth) {
  return ceilToMultiple(8, static_cast<int>(modWidth) + 6);
}

void writeModLoc(std::ostream &stream, const LogMod mod, const Loc loc) {
  std::ostringstream str;
  bool empty = true;
  if (!mod.empty()) {
    str << mod;
    empty = false;
  }
  if (loc.l != 0 || loc.c != 0) {
    if (!mod.empty()) {
      str << ':';
    }
    str << loc;
    empty = false;
  }
  if (!empty) {
    stream << std::left << std::setw(locationWidth(mod.size())) << str.str();
  }
}

}

stela::StreamSink::StreamSink()
  : StreamSink{std::cerr} {}

stela::StreamSink::StreamSink(const std::ostream &stream)
  : buf{stream.rdbuf()} {}

bool stela::StreamSink::writeHead(const LogInfo &head) {
  std::ostream stream{buf};
  writeModLoc(stream, head.mod, head.loc);
  stream << head.cat << ' ' << head.pri << ": ";
  return true;
}

std::streambuf *stela::StreamSink::getBuf(const LogInfo &) {
  return buf;
}

void stela::StreamSink::writeTail(const LogInfo &) {
  std::ostream stream{buf};
  stream << '\n';
  #ifndef NDEBUG
  stream << std::flush;
  #endif
}

namespace {

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
    UNREACHABLE();
  }
}

}

bool stela::ColorSink::writeHead(const LogInfo &head) {
  std::cerr << con::bold;
  writeModLoc(std::cerr, head.mod, head.loc);
  std::cerr << con::no_bold_faint;
  priorityColor(head.pri);
  std::cerr << head.cat << ' ' << head.pri;
  std::cerr << con::text_default << ": " << con::italic;
  return true;
}

std::streambuf *stela::ColorSink::getBuf(const LogInfo &) {
  return std::cerr.rdbuf();
}

void stela::ColorSink::writeTail(const LogInfo &) {
  std::cerr << con::no_italic << '\n';
  #ifndef NDEBUG
  std::cerr << std::flush;
  #endif
}

bool stela::NullSink::writeHead(const LogInfo &) {
  return false;
}

std::streambuf *stela::NullSink::getBuf(const LogInfo &) {
  UNREACHABLE();
}

void stela::NullSink::writeTail(const LogInfo &) {
  UNREACHABLE();
}

stela::FilterSink::FilterSink(LogSink &child, LogPri pri)
  : child{child}, priority{pri} {}

void stela::FilterSink::pri(const LogPri pri) {
  priority = pri;
}

bool stela::FilterSink::writeHead(const LogInfo &head) {
  if (static_cast<uint8_t>(head.pri) >= static_cast<uint8_t>(priority)) {
    return child.writeHead(head);
  } else {
    return false;
  }
}

std::streambuf *stela::FilterSink::getBuf(const LogInfo &head) {
  return child.getBuf(head);
}

void stela::FilterSink::writeTail(const LogInfo &head) {
  return child.writeTail(head);
}
