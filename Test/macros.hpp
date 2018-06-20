//
//  macros.hpp
//  STELA
//
//  Created by Indi Kernick on 20/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef macros_hpp
#define macros_hpp

#include <iostream>

#define STRINGIFY_IMPL(X) #X
#define STRINGIFY(X) STRINGIFY_IMPL(X)

#define PRINT_ERROR std::cout << "  " STRINGIFY(__LINE__) ": "

#define ASSERT_TRUE(EXP)                                                        \
  if (!(EXP)) {                                                                 \
    PRINT_ERROR "`" #EXP "` should be true\n";                                  \
    ++failCount;                                                                \
  } do{}while(0)
#define ASSERT_FALSE(EXP)                                                       \
  if (EXP) {                                                                    \
    PRINT_ERROR "`" #EXP "` should be false\n";                                 \
    ++failCount;                                                                \
  } do{}while(0)
#define ASSERT_EQ(EXP_A, EXP_B)                                                 \
  if ((EXP_A) != (EXP_B)) {                                                     \
    PRINT_ERROR "`" #EXP_A "` and `" #EXP_B "` should be equal\n";              \
    ++failCount;                                                                \
  } do{}while(0)
#define ASSERT_NEQ(EXP_A, EXP_B)                                                \
  if ((EXP_A) == (EXP_B)) {                                                     \
    PRINT_ERROR "`" #EXP_A "` and `" #EXP_B "` should not be equal\n";          \
    ++failCount;                                                                \
  } do{}while(0)

#define TEST(NAME, ...)                                                         \
  {                                                                             \
    std::cout << "Running test \"" << #NAME << "\"\n";                          \
    int failCount = 0;                                                          \
    { __VA_ARGS__ }                                                             \
    std::cout << "Test \"" << #NAME << "\" ";                                   \
    std::cout << (failCount == 0 ? "passed\n\n" : "failed\n\n");                \
  }

#endif
