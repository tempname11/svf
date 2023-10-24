#include <src/library.hpp>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include <generated/hpp/A0.hpp>

namespace schema = svf::A0;

int main(int /*argc*/, char */*argv*/[]) {
  // Fail, when logical compatibility is required, but no allocator is provided.
  {
    svf::runtime::Bytes message = {}; // Empty
    svf::runtime::Bytes scratch = {}; // Empty.

    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_logical
    );

    ASSERT(read_result.error_code == SVFRT_code_read__no_allocator_function);
  }

  return 0;
}
