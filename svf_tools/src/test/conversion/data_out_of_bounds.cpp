#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include "common.hpp"

int main(int /*argc*/, char */*argv*/[]) {
  auto arena_value = vm::create_linear_arena(1ull << 20);
  auto arena = &arena_value;
  PreparedSchemaParams dst_params = {
    .useq_type = svf::Meta::ConcreteType_tag::u64,
    .iseq_type = svf::Meta::ConcreteType_tag::i64,
    .fseq_type = svf::Meta::ConcreteType_tag::f64,
  };
  auto schema_dst = prepare_schema(arena, &dst_params);

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
  default_read_params.max_output_size = SVFRT_NO_SIZE_LIMIT;
  default_read_params.allocator_fn = allocate_arena;
  default_read_params.allocator_ptr = arena;

  // Fail, when the sequence offset is invalid.
  {
    PreparedSchemaParams prepare_params = { .change_leading_type = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    PreparedMessageParams message_params = {
      .sequence_count = 1,
      .invalid_sequence = true,
    };
    auto message = prepare_message(arena, &schema_src, &message_params);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_conversion__data_out_of_bounds);
  }

  // Fail, when the reference offset is invalid.
  {
    PreparedSchemaParams prepare_params = { .change_leading_type = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    PreparedMessageParams message_params = {
      .invalid_reference = true,
    };
    auto message = prepare_message(arena, &schema_src, &message_params);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_conversion__data_out_of_bounds);
  }

  // Fail, when one of the primitive sequence offsets is invalid.
  //
  // Note: different src-primitives trigger different code paths).

  svf::Meta::ConcreteType_tag utypes[] = {
    svf::Meta::ConcreteType_tag::u8,
    svf::Meta::ConcreteType_tag::u16,
    svf::Meta::ConcreteType_tag::u32,
    svf::Meta::ConcreteType_tag::u64,
  };

  svf::Meta::ConcreteType_tag itypes[] = {
    svf::Meta::ConcreteType_tag::i8,
    svf::Meta::ConcreteType_tag::i16,
    svf::Meta::ConcreteType_tag::i32,
    svf::Meta::ConcreteType_tag::i64,
  };

  svf::Meta::ConcreteType_tag ftypes[] = {
    svf::Meta::ConcreteType_tag::f32,
    svf::Meta::ConcreteType_tag::f64,
  };

  // U* types.
  for (UInt i = 0; i < sizeof(utypes) / sizeof(*utypes); i++) {
    auto type = utypes[i];

    PreparedSchemaParams prepare_params = {
      .change_leading_type = true,
      .useq_type = type,
    };
    auto schema_src = prepare_schema(arena, &prepare_params);
    PreparedMessageParams message_params = {
      .useq_count = 1,
      .useq_invalid = true,
    };
    auto message = prepare_message(arena, &schema_src, &message_params);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_conversion__data_out_of_bounds);
  }

  // I* types.
  for (UInt i = 0; i < sizeof(itypes) / sizeof(*itypes); i++) {
    auto type = itypes[i];

    PreparedSchemaParams prepare_params = {
      .change_leading_type = true,
      .iseq_type = type,
    };
    auto schema_src = prepare_schema(arena, &prepare_params);
    PreparedMessageParams message_params = {
      .iseq_count = 1,
      .iseq_invalid = true,
    };
    auto message = prepare_message(arena, &schema_src, &message_params);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_conversion__data_out_of_bounds);
  }

  // F* types.
  for (UInt i = 0; i < sizeof(ftypes) / sizeof(*ftypes); i++) {
    auto type = ftypes[i];

    PreparedSchemaParams prepare_params = {
      .change_leading_type = true,
      .fseq_type = type,
    };
    auto schema_src = prepare_schema(arena, &prepare_params);
    PreparedMessageParams message_params = {
      .fseq_count = 1,
      .fseq_invalid = true,
    };
    auto message = prepare_message(arena, &schema_src, &message_params);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_conversion__data_out_of_bounds);
  }

  // Fail, when the message is too short.
  {
    PreparedSchemaParams prepare_params = { .change_leading_type = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    auto message = prepare_message(arena, &schema_src, 0);
    message.count--;
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == SVFRT_code_conversion__data_out_of_bounds);
  }

  // TODO: `SVFRT_code_conversion__data_out_of_bounds` may actually occur in
  // 14 different places (as of current moment). It would be good to trigger
  // all of the reachable ones in different tests, maybe also differentiating
  // between them by some additional info other than the error code.

  // Success.
  {
    PreparedSchemaParams prepare_params = { .change_leading_type = true };
    auto schema_src = prepare_schema(arena, &prepare_params);
    auto message = prepare_message(arena, &schema_src, 0);
    SVFRT_ReadMessageResult read_result = {};
    SVFRT_read_message(&default_read_params, &read_result, message, scratch);
    ASSERT(read_result.error_code == 0);
  }

  return 0;
}
