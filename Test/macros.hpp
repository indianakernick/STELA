//
//  macros.hpp
//  STELA
//
//  Created by Indi Kernick on 20/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef macros_hpp
#define macros_hpp

#include <vector>
#include <iomanip>
#include <utility>
#include <iostream>
#include <string_view>

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
#define ASSERT_NE(EXP_A, EXP_B)                                                 \
  if ((EXP_A) == (EXP_B)) {                                                     \
    PRINT_ERROR "`" #EXP_A "` and `" #EXP_B "` should not be equal\n";          \
    ++failCount;                                                                \
  } do{}while(0)
#define ASSERT_THROWS(EXP, EXCEPTION)                                           \
  {                                                                             \
    bool caught = false;                                                        \
    try {                                                                       \
      EXP;                                                                      \
    } catch (EXCEPTION) {                                                       \
      caught = true;                                                            \
    } catch (...) {                                                             \
      PRINT_ERROR "`" #EXP "` should throw a `" #EXCEPTION "` exception\n"      \
        "  but threw something else\n";                                         \
      ++failCount;                                                              \
      caught = true;                                                            \
    }                                                                           \
    if (!caught) {                                                              \
      PRINT_ERROR "`" #EXP "` should throw a `" #EXCEPTION "` exception\n"      \
        "  but didn't throw\n";                                                 \
      ++failCount;                                                              \
    }                                                                           \
  } do{}while(0)
#define ASSERT_NOTHROW(EXP)                                                     \
  [&] {                                                                         \
    try {                                                                       \
      return (EXP);                                                             \
    } catch (...) {                                                             \
      PRINT_ERROR "`" #EXP "` should not throw an exception\n";                 \
      throw;                                                                    \
    }                                                                           \
  }()
#define ASSERT_DOWN_CAST(TYPE, PTR)                                             \
  [&] {                                                                         \
    if (PTR == nullptr) {                                                       \
      PRINT_ERROR "`" #PTR "` should not be a null pointer\n";                  \
      ++failCount;                                                              \
      throw 1;                                                                  \
    }                                                                           \
    TYPE *const newPtr = dynamic_cast<TYPE *>(PTR);                             \
    if (newPtr == nullptr) {                                                    \
      PRINT_ERROR "`" #PTR "` should have a dynamic type of `" #TYPE "`\n";            \
      ++failCount;                                                              \
      throw 1;                                                                  \
    }                                                                           \
    return newPtr;                                                              \
  }()

#define TEST(NAME, ...)                                                         \
  {                                                                             \
    std::cout << "  Running " << #NAME << "\n";                                 \
    int failCount = 0;                                                          \
    try {                                                                       \
      __VA_ARGS__                                                               \
    } catch (std::exception &e) {                                               \
      std::cout << "  EXCEPTION CAUGHT\n  " << e.what() << '\n';                \
      ++failCount;                                                              \
    } catch (...) {                                                             \
      std::cout << "  EXCEPTION CAUGHT\n";                                      \
      ++failCount;                                                              \
    }                                                                           \
    results.emplace_back(#NAME, failCount == 0);                                \
  }

#define TEST_GROUP(NAME, ...)                                                   \
  bool test##NAME() {                                                           \
    std::cout << "Testing " #NAME "\n";                                         \
    std::vector<std::pair<std::string_view, bool>> results;                     \
    { __VA_ARGS__ }                                                             \
    std::cout << '\n';                                                          \
    bool passedAll = true;                                                      \
    for (const auto &pair : results) {                                          \
      std::cout << std::left << std::setw(32) << pair.first;                    \
      if (pair.second) {                                                        \
        std::cout << "PASSED\n";                                                \
      } else {                                                                  \
        std::cout << "FAILED\n";                                                \
        passedAll = false;                                                      \
      }                                                                         \
    }                                                                           \
    std::cout << '\n';                                                          \
    if (passedAll) {                                                            \
      std::cout << "ALL PASSED!\n\n";                                           \
    }                                                                           \
    return passedAll;                                                           \
  }

#endif
