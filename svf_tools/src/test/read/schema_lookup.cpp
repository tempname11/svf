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

struct Control {
  bool should_fail;
};

SVFRT_Bytes schema_lookup(void *it, uint64_t schema_content_hash) {
  auto control = (Control *) it;
  if (!control->should_fail && schema_content_hash == schema::_SchemaDescription::content_hash) {
    return {
      .pointer = schema::_SchemaDescription::schema_binary_array,
      .count = schema::_SchemaDescription::schema_binary_size,
    };
  }
  return {};
}

int main(int /*argc*/, char */*argv*/[]) {
  // Prepare: create the message.
  auto arena_value = vm::create_linear_arena(1ull << 20);
  svf::runtime::WriteContext<schema::Entry> ctx = {};
  SVFRT_write_start(
    &ctx,
    write_arena,
    &arena_value,
    schema::_SchemaDescription::content_hash,
    {}, // Empty schema!
    {},
    schema::_SchemaDescription::PerType<schema::Entry>::type_id
  );
  schema::Entry entry = {};
  svf::runtime::write_finish(&ctx, &entry);

  ASSERT(ctx.finished);
  ASSERT(ctx.error_code == 0);

  auto message_pointer = arena_value.reserved_range.pointer;
  auto message_length = safe_int_cast<U32>(arena_value.waterline);

  // Fail with no lookup function.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact
    );

    ASSERT(read_result.error_code == SVFRT_code_read__no_schema_lookup_function);
  }

  // Fail, when the lookup fails.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    Control control = { .should_fail = true };
    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact,
      0, 0,
      schema_lookup, &control
    );

    ASSERT(read_result.error_code == SVFRT_code_read__schema_lookup_failed);
  }

  // Success.
  {
    svf::runtime::Bytes message = { message_pointer, message_length };
    svf::runtime::Bytes scratch = {}; // Empty.

    Control control = {};
    auto read_result = svf::runtime::read_message<schema::Entry>(
      message,
      scratch,
      svf::runtime::CompatibilityLevel::compatibility_exact,
      0, 0,
      schema_lookup, &control
    );

    ASSERT(read_result.error_code == 0);
  }

  return 0;
}
