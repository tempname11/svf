#pragma once
#include <cstdio>
#include "base.hpp"

class Impossible {
private:
  Impossible() {}
  friend Impossible terminate();

public:
  template<typename R>
  inline operator R () const { return operator R(); }
};

// Should never return.
Impossible terminate();

static inline
void report_error_details_default(char const *message) {
  printf("Internal error. Please report this.\n\n%s\n", message);
}

using ErrorReportFn = void (*) (char const *message);
extern ErrorReportFn _internal_report_error_details;

static inline
Impossible terminate_with_message(char const *message) {
  _internal_report_error_details(message);
  return terminate();
}

static inline
Impossible terminate_with_details(char const *reason, char const *filename, int line) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s at %s:%d." , reason, filename, line);
  return terminate_with_message(buffer);
}

// `ASSERT_CRITICAL` is always active, regardless of build mode.
#define ASSERT_CRITICAL(condition) { \
  if(!(condition)) { \
    (void) terminate_with_details( \
      ("Assertion (" #condition ") failed"), \
      __FILE__, \
      __LINE__ \
    ); \
  } \
}

// `ASSERT_OPTIONAL` matches the C `assert`, being only active in debug builds.
#ifdef NDEBUG
  #define ASSERT_OPTIONAL(condition)
#else
  #define ASSERT_OPTIONAL(condition) ASSERT_CRITICAL(condition)
#endif

// Deprecated, since it looks like C `assert`, but behaves differently.
//
// TODO: use `ASSERT_*` macros everywhere, instead.
//
#define ASSERT ASSERT_CRITICAL

#define UNREACHABLE terminate_with_details("Reached a code path which should be unreachable", __FILE__, __LINE__)
