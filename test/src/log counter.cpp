//
//  log counter.cpp
//  Test
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log counter.hpp"

using namespace stela;

CountLogs::CountLogs() {
  reset();
}

uint32_t CountLogs::countOf(const LogPri pri) const {
  return count[static_cast<size_t>(pri)];
}

void CountLogs::reset() {
  count.fill(0);
}

bool CountLogs::writeHead(const stela::LogInfo &info) {
  ++count[static_cast<size_t>(info.pri)];
  return false;
}

std::streambuf *CountLogs::getBuf(const stela::LogInfo &) {
  return silentBuf();
}

void CountLogs::writeTail(const stela::LogInfo &) {}
