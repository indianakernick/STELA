//
//  log counter.hpp
//  Test
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef test_log_counter_hpp
#define test_log_counter_hpp

#include <array>
#include <STELA/log.hpp>

class CountLogs final : public stela::LogSink {
public:
  CountLogs();
  
  uint32_t countOf(stela::LogPri) const;
  void reset();
  
private:
  std::array<uint32_t, static_cast<size_t>(stela::LogPri::nothing)> count;
  
  bool writeHead(const stela::LogInfo &) override;
  std::streambuf *getBuf(const stela::LogInfo &) override;
  void writeTail(const stela::LogInfo &) override;
};

#endif
