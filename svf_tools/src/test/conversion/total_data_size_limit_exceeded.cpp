#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include "common.hpp"

int main(int /*argc*/, char */*argv*/[]) {
  auto arena_value = vm::create_linear_arena(1ull << 20);
  auto arena = &arena_value;
  auto schema_dst = prepare_schema(arena, 0);

  U8 scratch_buffer[256];
  SVFRT_Bytes scratch = { .pointer = scratch_buffer, .count = sizeof(scratch_buffer) };

  SVFRT_ReadMessageParams default_read_params = {};
  default_read_params.expected_schema_content_hash = schema_dst.schema_content_hash;
  default_read_params.expected_schema_struct_strides = schema_dst.struct_strides;
  default_read_params.expected_schema = schema_dst.schema;
  default_read_params.required_level = SVFRT_compatibility_logical;
  default_read_params.entry_struct_id = schema_dst.entry_struct_id;
  default_read_params.entry_struct_index = 0;
  default_read_params.max_schema_work = UINT32_MAX;
  default_read_params.max_recursion_depth = SVFRT_DEFAULT_MAX_RECURSION_DEPTH;
  default_read_params.max_output_size = schema_dst.entry_stride;
  default_read_params.allocator_fn = allocate_arena;
  default_read_params.allocator_ptr = arena;

  // Fail, when resulting conversion data is too large.
  {
    PreparedSchemaParams prepare_params = { .change_leading_type = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    PreparedMessageParams message_params = { .sequence_count = 1 };
    auto message = prepare_message(arena, &schema_src, &message_params);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_conversion__total_data_size_limit_exceeded);
  }

  // Success (base case).
  {
    PreparedSchemaParams prepare_params = { .change_leading_type = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    auto message = prepare_message(arena, &schema_src, 0);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == 0);
  }

  // Success (larger output).
  {
    PreparedSchemaParams prepare_params = { .change_leading_type = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    PreparedMessageParams message_params = { .sequence_count = 1 };
    auto message = prepare_message(arena, &schema_src, &message_params);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;

    // TODO @proper-alignment.
    read_params.max_output_size = schema_dst.entry_stride + sizeof(SVFRT_Reference);

    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == 0);
  }

  return 0;
}
