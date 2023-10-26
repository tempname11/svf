#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_internal.h>
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
  default_read_params.required_level = SVFRT_compatibility_binary;
  default_read_params.entry_struct_id = schema_dst.entry_struct_id;
  default_read_params.entry_struct_index = 0;
  default_read_params.max_schema_work = UINT32_MAX;
  default_read_params.max_recursion_depth = SVFRT_DEFAULT_MAX_RECURSION_DEPTH;
  default_read_params.max_output_size = SVFRT_NO_SIZE_LIMIT;
  default_read_params.allocator_fn = allocate_arena;
  default_read_params.allocator_ptr = arena;

  // Fail, when structs are corrupted.
  {
    PreparedSchemaParams prepare_params = { .corrupt_structs = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__invalid_structs);
  }

  // Fail, when choices are corrupted.
  {
    PreparedSchemaParams prepare_params = { .corrupt_choices = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__invalid_choices);
  }

  // Fail, when fields are corrupted.
  {
    PreparedSchemaParams prepare_params = { .corrupt_fields = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__invalid_fields);
  }

  // Fail, when options are corrupted.
  {
    PreparedSchemaParams prepare_params = { .corrupt_options = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__invalid_options);
  }

  // Fail, when a struct index is corrupted.
  {
    PreparedSchemaParams prepare_params = { .corrupt_struct_index = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__invalid_struct_index);
  }

  // Fail, when a choice index is corrupted.
  {
    PreparedSchemaParams prepare_params = { .corrupt_choice_index = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__invalid_choice_index);
  }

  // Success for base case.
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
