#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include <src/svf_meta.hpp>
#include "../../core/common.hpp"
#include "common.hpp"

int main(int /*argc*/, char */*argv*/[]) {
  auto arena_value = vm::create_linear_arena(1ull << 20);
  auto arena = &arena_value;
  auto schema_dst = prepare_schema(arena, 0);

  U8 scratch_buffer[256];
  SVFRT_Bytes scratch = { .pointer = scratch_buffer, .count = sizeof(scratch_buffer) };
  U32 strides_dst[1] = { schema_dst.entry_stride };

  SVFRT_ReadMessageParams default_read_params = {};
  default_read_params.expected_schema_content_hash = schema_dst.schema_content_hash;
  default_read_params.expected_schema_struct_strides = { .pointer = strides_dst, .count = 1 };
  default_read_params.expected_schema = schema_dst.schema;
  default_read_params.required_level = SVFRT_compatibility_logical;
  default_read_params.entry_struct_name_hash = schema_dst.entry_struct_name_hash;
  default_read_params.entry_struct_index = 0;
  default_read_params.max_schema_work = 0;
  default_read_params.max_recursion_depth = SVFRT_DEFAULT_MAX_RECURSION_DEPTH;
  default_read_params.max_output_size = SVFRT_NO_SIZE_LIMIT;
  default_read_params.allocator_fn = allocate_arena;
  default_read_params.allocator_ptr = arena;

  // Fail on zero work limit.
  {
    // Use identical schemas.
    auto schema_src = schema_dst;
    schema_src.schema_content_hash += 1; // Ensure mismatch.
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__max_schema_work_exceeded);
  }

  // Fail with too many structs.
  {
    PreparedSchemaParams prepare_params = { .extra_structs = 1000 };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.max_schema_work = get_compatibility_work_base({
      schema_dst.schema.pointer,
      schema_dst.schema.count,
    }) * SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__max_schema_work_exceeded);
  }

  // Fail with too many fields.
  {
    PreparedSchemaParams prepare_params = { .extra_fields = 100 };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.max_schema_work = get_compatibility_work_base({
      schema_dst.schema.pointer,
      schema_dst.schema.count,
    }) * SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__max_schema_work_exceeded);
  }

  // Success.
  {
    PreparedSchemaParams prepare_params = { .extra_structs = 100, .extra_fields = 100 };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.max_schema_work = get_compatibility_work({
      schema_src.schema.pointer,
      schema_src.schema.count,
    }, {
      schema_dst.schema.pointer,
      schema_dst.schema.count,
    });
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == 0);
  }

  return 0;
}
