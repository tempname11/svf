#include <src/library.hpp>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include <generated/hpp/A0.hpp>

namespace schema = svf::A0;

U32 write_arena(void *it, SVFRT_Bytes src) {
  auto arena = (vm::LinearArena *) it;
  auto dst = vm::many<U8>(arena, src.count);
  range_copy(dst, {src.pointer, src.count});
  return safe_int_cast<U32>(src.count);
};

int main(int /*argc*/, char */*argv*/[]) {
  // Prepare: create the message.
  auto arena_value = vm::create_linear_arena(1ull << 20);
  auto ctx = svf::runtime::write_start<schema::Entry>(write_arena, &arena_value);
  schema::Entry entry = {};
  svf::runtime::write_finish(&ctx, &entry);

  ASSERT(ctx.finished);
  ASSERT(ctx.error_code == 0);

  auto message_pointer = arena_value.reserved_range.pointer;
  auto message_length = safe_int_cast<U32>(arena_value.waterline);

  // Fail on empty message.
  {
    svf::runtime::Bytes message = {}; // Empty.
    svf::runtime::Bytes scratch = {}; // Empty.

    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == SVFRT_code_read__header_too_small);
  }

  // Fail on misaligned message.
  {
    svf::runtime::Bytes message = { message_pointer + 1, message_length - 1 };
    svf::runtime::Bytes scratch = {}; // Empty.

    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == SVFRT_code_read__header_not_aligned);
  }

  // Fail on mangled magic.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    auto header = (SVFRT_MessageHeader *) message.pointer;

    // Mangle the magic.
    header->magic[0] = 'X';
    header->magic[1] = 'Y';
    header->magic[2] = 'Z';

    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == SVFRT_code_read__header_magic_mismatch);

    // Restore the magic.
    header->magic[0] = 'S';
    header->magic[1] = 'V';
    header->magic[2] = 'F';
  }

  // Fail on mangled version.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    auto header = (SVFRT_MessageHeader *) message.pointer;

    // Mangle the version.
    auto old_version = header->version;
    header->version = 255;

    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == SVFRT_code_read__header_version_mismatch);

    // Restore the version.
    header->version = old_version;
  }

  // Fail when trying to read a different entry.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    auto read_result = svf::runtime::read_message<schema::SomeStruct>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == SVFRT_code_read__entry_struct_id_mismatch);
  }

  // Fail when schema is specified as out of bounds.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    auto header = (SVFRT_MessageHeader *) message.pointer;

    // Modify the length.
    auto old_length = header->schema_length;
    header->schema_length = 2 * message_length;

    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == SVFRT_code_read__bad_schema_length);

    // Restore the length.
    header->schema_length = old_length;
  }

  // Fail when data is zero-length.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    auto header = (SVFRT_MessageHeader *) message.pointer;

    message.count = sizeof(*header) + header->schema_length;
    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == SVFRT_code_read__bad_schema_length);
  }

  // Success.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == 0);
  }

  return 0;
}
