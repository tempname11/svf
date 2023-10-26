#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include <src/svf_meta.hpp>
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
  default_read_params.entry_struct_id = schema_dst.entry_struct_id;
  default_read_params.entry_struct_index = 0;
  default_read_params.max_schema_work = SVFRT_NO_SIZE_LIMIT;
  default_read_params.max_recursion_depth = SVFRT_DEFAULT_MAX_RECURSION_DEPTH;
  default_read_params.max_output_size = SVFRT_NO_SIZE_LIMIT;
  default_read_params.allocator_fn = allocate_arena;
  default_read_params.allocator_ptr = arena;

  // Fail, when `required_level` is 0.
  {
    // Use identical schemas.
    auto schema_src = schema_dst;
    schema_src.schema_content_hash += 1; // Ensure mismatch.
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.required_level = SVFRT_compatibility_none;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__required_level_is_none);
  }

  // Fail, when the src-schema is too small.
  {
    // Use identical schemas.
    auto schema_src = schema_dst;
    schema_src.schema_content_hash += 1; // Ensure mismatch.
    schema_src.schema.count = sizeof(svf::Meta::SchemaDefinition) - 1; // Truncate to cause failure.
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__schema_too_small);
  }

  // Fail, when the dst-schema is too small.
  {
    // Use identical schemas.
    auto schema_src = schema_dst;
    schema_src.schema_content_hash += 1; // Ensure mismatch.
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.expected_schema.count = sizeof(svf::Meta::SchemaDefinition) - 1; // Truncate to cause failure.
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility_internal__schema_too_small);
  }

  // Fail, when no scratch memory is provided.
  {
    // Use identical schemas.
    auto schema_src = schema_dst;
    schema_src.schema_content_hash += 1; // Ensure mismatch.
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_Bytes no_scratch = {};
    SVFRT_read_message(&default_read_params, &read_result, message, no_scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__not_enough_scratch_memory);
  }

  // Fail, when an unknown entry-struct name-hash is provided.
  {
    // Use identical schemas.
    auto schema_src = schema_dst;
    schema_src.schema_content_hash += 1; // Ensure mismatch.
    schema_src.entry_struct_id = 0xDEAD; // Force header to contain the new hash.
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.entry_struct_id = 0xDEAD;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__entry_struct_id_not_found);
  }

  // Success.
  {
    // Use identical schemas.
    auto schema_src = schema_dst;
    schema_src.schema_content_hash += 1; // Ensure mismatch.
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == 0);
  }

  return 0;
}
