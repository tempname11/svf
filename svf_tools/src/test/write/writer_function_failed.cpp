#include <src/library.hpp>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include <generated/hpp/A0.hpp>

namespace schema = svf::A0;

struct TestWriter {
  Bool should_fail;
};

uint32_t test_write(void *it, SVFRT_Bytes data) {
  auto writer = (TestWriter *) it;
  return writer->should_fail ? 0 : data.count;
}

int main(int /*argc*/, char */*argv*/[]) {
  // Fail at start.
  {
    auto writer = TestWriter {};
    writer.should_fail = true;

    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, &writer);
    ASSERT(ctx.error_code == SVFRT_code_write__writer_function_failed);
  }

  // Fail on reference write.
  {
    auto writer = TestWriter {};

    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, &writer);
    ASSERT(ctx.error_code == 0);

    writer.should_fail = true;
    schema::SomeStruct x = {};
    svf::runtime::write_reference(&ctx, &x);
    ASSERT(ctx.error_code == SVFRT_code_write__writer_function_failed);
  }

  // Fail on sequence write.
  {
    auto writer = TestWriter {};

    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, &writer);
    ASSERT(ctx.error_code == 0);

    writer.should_fail = true;
    schema::SomeStruct x = {};
    svf::runtime::write_sequence(&ctx, &x, 1);
    ASSERT(ctx.error_code == SVFRT_code_write__writer_function_failed);
  }

  // Fail on sequence element write.
  {
    auto writer = TestWriter {};

    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, &writer);
    ASSERT(ctx.error_code == 0);

    writer.should_fail = true;
    schema::SomeStruct x = {};
    svf::Sequence<schema::SomeStruct> sequence = {};
    svf::runtime::write_sequence_element(&ctx, &x, &sequence);
    ASSERT(ctx.error_code == SVFRT_code_write__writer_function_failed);
  }

  // Success.
  {
    auto writer = TestWriter {};

    auto ctx = svf::runtime::write_start<schema::Entry>(test_write, &writer);
    ASSERT(ctx.error_code == 0);

    schema::SomeStruct x = {};
    svf::runtime::write_reference(&ctx, &x);
    svf::runtime::write_sequence(&ctx, &x, 1);

    svf::Sequence<schema::SomeStruct> sequence = {};
    svf::runtime::write_sequence_element(&ctx, &x, &sequence);

    ASSERT(ctx.error_code == 0);
  }

  return 0;
}
