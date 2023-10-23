#include <src/library.hpp>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include <generated/hpp/A0.hpp>

namespace schema = svf::A0;

uint32_t test_write(void *, SVFRT_Bytes data) { return data.count; }

int main(int /*argc*/, char */*argv*/[]) {
  // Fail.
  {
    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, 0);

    schema::SomeStruct x = {};
    svf::Sequence<schema::SomeStruct> sequence = {};

    svf::runtime::write_sequence_element(&ctx, &x, &sequence);
    ASSERT(ctx.error_code == 0);

    // Break up the sequence.
    svf::runtime::write_reference(&ctx, &x);

    svf::runtime::write_sequence_element(&ctx, &x, &sequence);
    ASSERT(ctx.error_code == SVFRT_code_write__sequence_non_contiguous);
  }

  // Success.
  {
    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, 0);

    schema::SomeStruct x = {};
    svf::Sequence<schema::SomeStruct> sequence = {};

    svf::runtime::write_sequence_element(&ctx, &x, &sequence);
    ASSERT(ctx.error_code == 0);

    svf::runtime::write_sequence_element(&ctx, &x, &sequence);
    ASSERT(ctx.error_code == 0);
  }

  return 0;
}
