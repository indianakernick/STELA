//
//  log counter.hpp
//  Test
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef test_log_counter_hpp
#define test_log_counter_hpp

#include <STELA/log.hpp>

class CountLogs final : public stela::LogSink {
public:
  CountLogs() = default;
  
  uint32_t status() const;
  uint32_t verbose() const;
  uint32_t info() const;
  uint32_t warn() const;
  uint32_t error() const;
  void reset();
  
private:
  uint32_t statusCount = 0;
  uint32_t verboseCount = 0;
  uint32_t infoCount = 0;
  uint32_t warnCount = 0;
  uint32_t errorCount = 0;
  
  bool writeHead(const stela::LogInfo &) override;
  std::streambuf *getBuf(const stela::LogInfo &) override;
  void writeTail(const stela::LogInfo &) override;
};

#endif
