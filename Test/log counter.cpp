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

std::ostream &CountLogs::log(stela::LogCat, const stela::LogPri pri, stela::Loc) {
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
  return stela::silentStream();
}

void CountLogs::endLog(stela::LogCat, stela::LogPri) {}
