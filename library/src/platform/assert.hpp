#pragma once
#include "base.hpp"

// Forcefully abort the process, printing the source code location if provided.
void abort_this_process(
  char const *message = 0,
  char const *filename = 0,
  UInt line = 0
);

// Unlike C `assert`, this macro is always enabled. We consider assertions fatal,
// and it's best to quit than continue in an invalid state. The cost of this
// check is negligible, and it's best to catch bugs early.
#define ASSERT(condition) {if(!(condition)) abort_this_process(#condition, __FILE__, __LINE__);}
