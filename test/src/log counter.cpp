//
//  log counter.cpp
//  Test
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log counter.hpp"

#include <cassert>

using namespace stela;

uint32_t CountLogs::verbose() const {
  return verboseCount;
}

uint32_t CountLogs::info() const {
  return infoCount;
}

uint32_t CountLogs::warn() const {
  return warnCount;
}

uint32_t CountLogs::error() const {
  return errorCount;
}

void CountLogs::reset() {
  verboseCount = infoCount = warnCount = errorCount = 0;
}

std::streambuf *CountLogs::getBuf(LogCat, LogPri) {
  return silentBuf();
}

void CountLogs::begin(const LogCat cat, const LogPri pri, const LogMod mod, Loc) {
  begin(cat, pri, mod);
}

void CountLogs::begin(LogCat, const LogPri pri, LogMod) {
  switch (pri) {
    case LogPri::verbose:
      ++verboseCount;
      break;
    case LogPri::info:
      ++infoCount;
      break;
    case LogPri::warning:
      ++warnCount;
      break;
    case LogPri::error:
      ++errorCount;
      break;
    default:
      assert(false);
  }
}

void CountLogs::end(LogCat, LogPri) {}
