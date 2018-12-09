//
//  log counter.cpp
//  Test
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log counter.hpp"

using namespace stela;

uint32_t CountLogs::status() const {
  return statusCount;
}

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
  statusCount = verboseCount = infoCount = warnCount = errorCount = 0;
}

bool CountLogs::writeHead(const stela::LogInfo &info) {
  switch (info.pri) {
    case LogPri::status:
      ++statusCount;
      break;
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
    case LogPri::nothing: ;
  }
  return false;
}

std::streambuf *CountLogs::getBuf(const stela::LogInfo &) {
  return silentBuf();
}

void CountLogs::writeTail(const stela::LogInfo &) {}
