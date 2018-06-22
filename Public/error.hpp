//
//  error.hpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_error_hpp
#define stela_error_hpp

#include <exception>

namespace stela {

using Line = uint32_t;
using Col = uint32_t;

class Error : public std::exception {
public:
  Error(const char *, Line, Col, const char *);
  
  Line line() const {
    return line_;
  }
  Col col() const {
    return col_;
  }
  
  const char *what() const noexcept override;
  
private:
  const char *type;
  const char *msg;
  Line line_;
  Col col_;
};

}

#endif
