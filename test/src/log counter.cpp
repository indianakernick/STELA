//
//  log counter.cpp
//  Test
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log counter.hpp"

uint32_t CountLogs::infos() const {
  return infoCount;
}

uint32_t CountLogs::warnings() const {
  return warnCount;
}

uint32_t CountLogs::errors() const {
  return errorCount;
}

void CountLogs::reset() {
  infoCount = warnCount = errorCount = 0;
}

std::streambuf *CountLogs::getBuf(stela::LogCat, stela::LogPri) {
  return stela::silentBuf();
}

void CountLogs::begin(const stela::LogCat cat, const stela::LogPri pri, stela::Loc) {
  begin(cat, pri);
}

void CountLogs::begin(stela::LogCat, const stela::LogPri pri) {
  switch (pri) {
    case stela::LogPri::info:
      ++infoCount;
      break;
    case stela::LogPri::warning:
      ++warnCount;
      break;
    case stela::LogPri::error:
      ++errorCount;
      break;
    default: ;
  }
}

void CountLogs::end(stela::LogCat, stela::LogPri) {}
