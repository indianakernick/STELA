//
//  unreachable.hpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef engine_unreachable_hpp
#define engine_unreachable_hpp

#if defined(NDEBUG) && (defined(__GNUC__) || defined(__clang__))

#pragma message "Calling __builtin_unreachable in unreachable contexts"
#define UNREACHABLE() __builtin_unreachable()

#else

#pragma message "Calling std::terminate in unreachable contexts"
#include <exception>
#define UNREACHABLE() std::terminate()

#endif

#endif
