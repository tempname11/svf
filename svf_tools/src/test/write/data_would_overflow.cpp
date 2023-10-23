#include <src/library.hpp>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include <generated/hpp/A0.hpp>

namespace schema = svf::A0;

uint32_t test_write(void *, SVFRT_Bytes data) { return data.count; }

int main(int /*argc*/, char */*argv*/[]) {
  // Overflow immediately.
  {
    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, 0);
    schema::SomeStruct x = {};

    // Note: strictly speaking, this is UB, as this is a pointer to a single
    // object, yet we reference `UINT32_MAX` of them. But since the pointed-to
    // data is never accessed, it should be fine on systems with 64-bit pointers.

    svf::runtime::write_sequence(&ctx, &x, UINT32_MAX / sizeof(schema::SomeStruct) + 1);
    ASSERT(ctx.error_code == SVFRT_code_write__data_would_overflow);
  }

  // Overflow on second call.
  {
    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, 0);
    schema::SomeStruct x = {};

    svf::runtime::write_sequence(&ctx, &x, (UINT32_MAX / sizeof(schema::SomeStruct) + 1) / 2);
    ASSERT(ctx.error_code == 0);

    svf::runtime::write_sequence(&ctx, &x, (UINT32_MAX / sizeof(schema::SomeStruct) + 1) / 2);
    ASSERT(ctx.error_code == SVFRT_code_write__data_would_overflow);
  }

  // Success.
  {
    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, 0);
    schema::SomeStruct x = {};

    svf::runtime::write_sequence(&ctx, &x, (UINT32_MAX / sizeof(schema::SomeStruct) + 1) / 3);
    ASSERT(ctx.error_code == 0);

    svf::runtime::write_sequence(&ctx, &x, (UINT32_MAX / sizeof(schema::SomeStruct) + 1) / 3);
    ASSERT(ctx.error_code == 0);
  }

  return 0;
}
