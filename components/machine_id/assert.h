// extracted from rlz lib

#ifndef CHROMIUM_ASSERT_H
#define CHROMIUM_ASSERT_H

#include <string>

#include "base/logging.h"

// An assertion macro.
// Can mute expected assertions in debug mode.

#ifndef ASSERT_STRING
#ifndef MUTE_EXPECTED_ASSERTS
#define ASSERT_STRING(expr) LOG_IF(FATAL, false) << (expr)
#else
#define ASSERT_STRING(expr)                               \
  do {                                                    \
    std::string expr_string(expr);                        \
    if (machine_id::expected_assertion_ != expr_string) { \
      LOG_IF(FATAL, false) << (expr);                     \
    }                                                     \
  } while (0)
#endif
#endif

#ifndef VERIFY
#ifdef _DEBUG
#define VERIFY(expr) LOG_IF(FATAL, !(expr)) << #expr
#else
#define VERIFY(expr) (void)(expr)
#endif
#endif

namespace machine_id {

#ifdef MUTE_EXPECTED_ASSERTS
extern std::string expected_assertion_;
#endif

inline void SetExpectedAssertion(const char* s) {
#ifdef MUTE_EXPECTED_ASSERTS
  expected_assertion_ = s;
#endif
}

}  // namespace machine_id
#endif  // CHROMIUM_ASSERT_H
