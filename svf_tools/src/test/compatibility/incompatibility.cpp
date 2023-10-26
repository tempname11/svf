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
  default_read_params.entry_struct_name_hash = schema_dst.entry_struct_name_hash;
  default_read_params.entry_struct_index = 0;
  default_read_params.max_schema_work = UINT32_MAX;
  default_read_params.max_recursion_depth = SVFRT_DEFAULT_MAX_RECURSION_DEPTH;
  default_read_params.max_output_size = SVFRT_NO_SIZE_LIMIT;
  default_read_params.allocator_fn = allocate_arena;
  default_read_params.allocator_ptr = arena;

  // Fail, when src-entry-struct has less fields than dst-entry-struct.
  {
    PreparedSchemaParams prepare_params = { .less_fields = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__field_is_missing);
  }

  // Fail, when src-entry-struct has different fields to dst-entry-struct.
  {
    PreparedSchemaParams prepare_params = { .change_field_name_hashes = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__field_is_missing);
  }

  // Fail, when a reachable src-choice has more options than the corresponding dst-choice.
  {
    PreparedSchemaParams prepare_params = { .extra_options = 1 };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__option_is_missing);
  }

  // Fail, when a reachable src-choice has different options to the corresponding dst-choice.
  {
    PreparedSchemaParams prepare_params = { .change_option_name_hashes = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__option_is_missing);
  }

  // Fail, when a field offset has changed (and binary compatibility is required).
  {
    PreparedSchemaParams prepare_params = { .change_field_offsets = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__field_offset_mismatch);
  }

  // Success, when a field offset has changed, but only logical compatibility is required.
  {
    PreparedSchemaParams prepare_params = { .change_field_offsets = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.required_level = SVFRT_compatibility_logical;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == 0);
  }

  // Fail, when an option tag has changed (and binary compatibility is required).
  {
    PreparedSchemaParams prepare_params = { .change_option_tags = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__option_tag_mismatch);
  }

  // Success, when an option tag has changed, but only logical compatibility is required.
  {
    PreparedSchemaParams prepare_params = { .change_option_tags = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.required_level = SVFRT_compatibility_logical;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == 0);
  }

  // Fail, when a type tag has changed.
  {
    PreparedSchemaParams prepare_params = { .change_type_tag = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__type_mismatch);
  }

  // Fail, when a concrete type tag has changed.
  {
    PreparedSchemaParams prepare_params = { .change_concrete_type_tag = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__concrete_type_mismatch);
  }

  // Fail, when a concrete type tag has changed.
  {
    PreparedSchemaParams prepare_params = { .change_concrete_type_tag = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__concrete_type_mismatch);
  }

  // Fail, when src-struct size is less than the corresponding dst-struct size
  // (and binary compatibility is required).
  {
    PreparedSchemaParams prepare_params = { .no_struct_end_padding = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__struct_size_mismatch);
  }

  // Success, when src-struct size is greater than than the corresponding
  // dst-struct size (and binary compatibility is required).
  {
    PreparedSchemaParams prepare_params = { .extra_struct_end_padding = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == 0);
  }

  // Fail, when src-struct size is greater than than the corresponding dst-struct size,
  // and exact compatibility is required.
  {
    PreparedSchemaParams prepare_params = { .extra_struct_end_padding = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_ReadMessageParams read_params = default_read_params;
    read_params.required_level = SVFRT_compatibility_exact;
    SVFRT_read_message(&read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__struct_size_mismatch);
  }

  // Fail, when two fields that refer to different src-structs, match fields that
  // refer to the same dst-struct. See comment below.
  {
    PreparedSchemaParams prepare_params = { .different_struct_refs = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__struct_index_mismatch);
  }

  // Fail, when two fields that refer to different src-choices, match fields that
  // refer to the same dst-choice. See comment below.
  {
    PreparedSchemaParams prepare_params = { .different_choice_refs = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    SVFRT_Bytes message = prepare_message(arena, &schema_src);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_compatibility__choice_index_mismatch);
  }

  // TODO: test the inverse of the above, namely that when fields that refer to
  // the same src-struct/src-choice, match fields that refer to different
  // dst-structs/dst-choices, it is completely fine.
  //
  // In other words, values that were the same custom type, are allowed to
  // become different custom types. But values that were different custom types,
  // can't become same custom type. For binary compatibility, that is indeed
  // impossible, because then the stride information would be lost.
  //
  // For logical compatibility, that is not a problem, so this limitation might
  // be lifted in the future, but it looks as if that will require
  // dynamically-sized `unknown_src_N * known_dst_M` scratch memory, versus just
  // fixed-size `known_dst_M`. It can be argued, that, for simplicity and
  // consistency reasons, this limitation should be present regardless of the
  // required compatibility level.

  // Success (base case).
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
