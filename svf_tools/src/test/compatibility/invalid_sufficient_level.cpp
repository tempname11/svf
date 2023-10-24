#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_internal.h>
#include "common.hpp"

int main(int /*argc*/, char */*argv*/[]) {
  SVFRT_CompatibilityResult result;
  SVFRT_check_compatibility(
    &result,
    {}, {}, {}, 0,
    SVFRT_compatibility_exact,
    SVFRT_compatibility_binary,
    0
  );

  ASSERT(result.error_code == SVFRT_code_compatibility__invalid_sufficient_level);

  return 0;
}
