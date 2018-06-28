//
//  log counter.cpp
//  Test
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log counter.hpp"

#include <cassert>

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

uint32_t CountLogs::fatalError() const {
  return fatalErrorCount;
}

void CountLogs::reset() {
  verboseCount = infoCount = warnCount = errorCount = fatalErrorCount = 0;
}

std::streambuf *CountLogs::getBuf(stela::LogCat, stela::LogPri) {
  return stela::silentBuf();
}

void CountLogs::begin(const stela::LogCat cat, const stela::LogPri pri, stela::Loc) {
  begin(cat, pri);
}

void CountLogs::begin(stela::LogCat, const stela::LogPri pri) {
  switch (pri) {
    case stela::LogPri::verbose:
      ++verboseCount;
      break;
    case stela::LogPri::info:
      ++infoCount;
      break;
    case stela::LogPri::warning:
      ++warnCount;
      break;
    case stela::LogPri::error:
      ++errorCount;
      break;
    case stela::LogPri::fatal_error:
      ++fatalErrorCount;
      break;
    default:
      assert(false);
  }
}

void CountLogs::end(stela::LogCat, stela::LogPri) {}
