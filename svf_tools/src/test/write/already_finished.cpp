#include <src/library.hpp>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include <generated/hpp/A0.hpp>

namespace schema = svf::A0;

uint32_t test_write(void *, SVFRT_Bytes data) { return data.count; }

int main(int /*argc*/, char */*argv*/[]) {
  // Fail on write-after-finish.
  {
    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, 0);
    auto entry = schema::Entry {};

    svf::runtime::write_finish(&ctx, &entry);
    ASSERT(ctx.error_code == 0);
    ASSERT(ctx.finished == true);

    svf::runtime::write_finish(&ctx, &entry);
    ASSERT(ctx.error_code == SVFRT_code_write__already_finished);
    ASSERT(ctx.finished == true);
  }

  return 0;
}
