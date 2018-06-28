//
//  log counter.hpp
//  Test
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef log_counter_hpp
#define log_counter_hpp

#include <STELA/log.hpp>

class CountLogs final : public stela::LogBuf {
public:
  CountLogs() = default;
  
  uint32_t verbose() const;
  uint32_t info() const;
  uint32_t warn() const;
  uint32_t error() const;
  uint32_t fatalError() const;
  void reset();
  
private:
  uint32_t verboseCount = 0;
  uint32_t infoCount = 0;
  uint32_t warnCount = 0;
  uint32_t errorCount = 0;
  uint32_t fatalErrorCount = 0;
  
  std::streambuf *getBuf(stela::LogCat, stela::LogPri) override;
  void begin(stela::LogCat, stela::LogPri, stela::Loc) override;
  void begin(stela::LogCat, stela::LogPri) override;
  void end(stela::LogCat, stela::LogPri) override;
};

#endif
