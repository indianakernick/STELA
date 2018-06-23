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

class CountLogs final : public stela::Log {
public:
  CountLogs() = default;
  
  // informations!
  uint32_t infos() const;
  uint32_t warnings() const;
  uint32_t errors() const;
  void reset();
  
private:
  uint32_t infoCount = 0;
  uint32_t warnCount = 0;
  uint32_t errorCount = 0;
  
  std::ostream &log(stela::LogCat, stela::LogPri, stela::Loc) override;
  std::ostream &log(stela::LogCat, stela::LogPri) override;
  void endLog(stela::LogCat, stela::LogPri) override;
};

#endif
