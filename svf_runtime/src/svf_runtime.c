#ifndef SVFRT_SINGLE_FILE
  #include "svf_internal.h"
  #include "svf_runtime.h"
  #include "svf_meta.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline
uint64_t SVFRT_align_down(uint64_t value, uint64_t alignment) {
  return value - value % alignment;
}

static inline
uint64_t SVFRT_align_up(uint64_t value, uint64_t alignment) {
  return SVFRT_align_down(value - 1, alignment) + alignment;
}

// TODO: error reporting (see all the `printf` placeholders). Probably should
// just return an enum?
void SVFRT_read_message_implementation(
  SVFRT_ReadMessageResult *out_result,
  SVFRT_Bytes message_bytes,
  uint64_t entry_name_hash,
  uint32_t entry_index,
  SVFRT_Bytes expected_schema,
  uint32_t work_max,
  uint32_t max_recursion_depth,
  uint32_t max_output_size,
  SVFRT_Bytes scratch_memory,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_AllocatorFn *allocator_fn,
  void *allocator_ptr
) {
  out_result->entry = NULL;
  out_result->allocation = NULL;
  out_result->compatibility_level = SVFRT_compatibility_none;

  if (message_bytes.count < sizeof(SVFRT_MessageHeader)) {
    // printf("Too small for an SVF header.\n");
    return;
  }

  if (((uintptr_t) message_bytes.pointer) % SVFRT_MESSAGE_PART_ALIGNMENT != 0) {
    // printf("Header not aligned.\n");
    return;
  }

  SVFRT_MessageHeader *header = (SVFRT_MessageHeader *) message_bytes.pointer;
  if (0
    || header->magic[0] != 'S'
    || header->magic[1] != 'V'
    || header->magic[2] != 'F'
  ) {
    // printf("Does not match SVF header magic.\n");
    return;
  }

  // For now, versions must match exactly. Version 0 is for development only and
  // does not come with any guarantees.
  if (header->version != 0) {
    // printf("Does not match SVF version 0.\n");
    return;
  }

  // Make sure the declared entry is the same as we expect.
  if (header->entry_name_hash != entry_name_hash) {
    // printf("Entry name hash does not match.\n");
    return;
  }

  // Make sure we can read the schema.
  if (header->schema_length < sizeof(SVF_META_Schema)) {
    // printf("Schema is too small.\n");
    return;
  }

  // Prevent addition overflow by casting operands to `uint64_t` first.
  uint64_t schema_padded_end_offset = SVFRT_align_up(
    (uint64_t) sizeof(SVFRT_MessageHeader) + (uint64_t) (header->schema_length),
    SVFRT_MESSAGE_PART_ALIGNMENT
  );

  // Make sure the schema (and additional padding after it) is in-bounds.
  // This also implies it is less than `UINT32_MAX`.
  if (schema_padded_end_offset > (uint64_t) message_bytes.count) {
    // printf("Schema is not in-bounds.\n");
    return;
  }

  // We now have a valid schema and data ranges. The data range is implicit,
  // from the padded end of the schema, to the end of the message.
  SVFRT_Bytes schema_range = {
    /*.pointer =*/ message_bytes.pointer + sizeof(SVFRT_MessageHeader),
    /*.count =*/ header->schema_length,
  };
  SVFRT_Bytes data_range = {
    /*.pointer =*/ message_bytes.pointer + schema_padded_end_offset,
    /*.count =*/ message_bytes.count - schema_padded_end_offset,
  };

  SVFRT_CompatibilityResult check_result = {0};
  SVFRT_check_compatibility(
    &check_result,
    scratch_memory,
    schema_range,
    expected_schema,
    entry_name_hash,
    required_level,
    SVFRT_compatibility_exact,
    work_max
  );

  if (check_result.level == 0 || check_result.error_code != 0) {
    // printf("Compatibility error.\n");
    return;
  }

  SVFRT_Bytes final_data_range = data_range;
  if (check_result.level == SVFRT_compatibility_logical) {
    // We have to convert the message.

    if (!allocator_fn) {
      // printf("Need an allocator to convert the message.\n");
      return;
    }

    SVFRT_ConversionResult conversion_result = {0};

    SVFRT_Bytes unsafe_schema_src = schema_range;
    SVFRT_Bytes schema_dst = expected_schema;

    // TODO @proper-alignment.
    SVF_META_Schema *unsafe_definition_src = (SVF_META_Schema *) (unsafe_schema_src.pointer + unsafe_schema_src.count - sizeof(SVF_META_Schema));
    SVF_META_Schema *definition_dst = (SVF_META_Schema *) (schema_dst.pointer + schema_dst.count - sizeof(SVF_META_Schema));

    SVFRT_ConversionInfo conversion_info = {
      .unsafe_schema_src = unsafe_schema_src,
      .schema_dst = schema_dst,
      .unsafe_definition_src = unsafe_definition_src,
      .definition_dst = definition_dst,
      .struct_index_src = check_result.entry_struct_index_src,
      .struct_index_dst = check_result.entry_struct_index_dst,
    };

    uint32_t unsafe_entry_size = check_result.unsafe_entry_size_src;

    if (unsafe_entry_size > data_range.count) {
      // printf("Conversion was not successful, because...");
      return;
    }

    // Now, `unsafe_entry_size` can be considered safe.
    // TODO @proper-alignment.
    SVFRT_Bytes entry_input_bytes = {
      /*.pointer =*/ data_range.pointer + data_range.count - unsafe_entry_size,
      /*.count =*/ unsafe_entry_size,
    };

    SVFRT_convert_message(
      &conversion_result,
      &conversion_info,
      entry_input_bytes,
      data_range,
      max_recursion_depth,
      max_output_size,
      allocator_fn,
      allocator_ptr
    );

    if (!conversion_result.success) {
      out_result->allocation = conversion_result.output_bytes.pointer;
      // out_result->error_code = conversion_result->error_code;
      return;
    }

    final_data_range = conversion_result.output_bytes;
  }

  // For at least binary compatibility, this will be the size of the entry in
  // the "from" schema. However, for logical compatibility, this will be the size
  // of the entry in the "to" schema.
  //
  // This is non-obvious. See #logical-compatibility-stride-quirk.
  uint32_t entry_size = check_result.quirky_struct_strides_dst.pointer[entry_index];

  if (final_data_range.count < entry_size) {
    // printf("Data is too small.\n");
    return;
  }

  uint32_t entry_alignment = 1; // TODO @proper-alignment.
  uint32_t final_entry_offset = (uint32_t) SVFRT_align_down(
    final_data_range.count - entry_size,
    entry_alignment
  );

  out_result->entry = (void *) (final_data_range.pointer + final_entry_offset);
  if (check_result.level == SVFRT_compatibility_logical) {
    out_result->allocation = final_data_range.pointer;
  }
  out_result->compatibility_level = check_result.level;
  out_result->context.data_range = final_data_range;
  out_result->context.struct_strides = check_result.quirky_struct_strides_dst;
}

void SVFRT_write_message_implementation(
  SVFRT_WriteContext *result,
  void *writer_ptr,
  SVFRT_WriterFn *writer_fn,
  SVFRT_Bytes schema_bytes,
  uint64_t entry_name_hash
) {
  SVFRT_MessageHeader header = {
    .magic = { 'S', 'V', 'F' },
    .version = 0,
    .schema_length = (uint32_t) schema_bytes.count,
    .entry_name_hash = entry_name_hash,
  };

  SVFRT_Bytes header_bytes = {
    /*.pointer =*/ (uint8_t *) &header,
    /*.count =*/ sizeof(header)
  };

  bool error = false;

  uint32_t written_header = writer_fn(writer_ptr, header_bytes);
  if (written_header != header_bytes.count) {
    error = true;
  }

  uint32_t written_schema = writer_fn(writer_ptr, schema_bytes);
  if (written_schema != schema_bytes.count) {
    error = true;
  }

  uint8_t zeros[SVFRT_MESSAGE_PART_ALIGNMENT] = {0};
  uint32_t misaligned = written_schema % SVFRT_MESSAGE_PART_ALIGNMENT;
  SVFRT_Bytes padding_bytes = {
    /*.pointer =*/ (uint8_t *) zeros,
    /*.count =*/ SVFRT_MESSAGE_PART_ALIGNMENT - misaligned,
  };
  if (misaligned != 0) {
    uint32_t written_padding = writer_fn(writer_ptr, padding_bytes);
    if (written_padding != padding_bytes.count) {
      error = true;
    }
  }

  result->writer_ptr = writer_ptr;
  result->writer_fn = writer_fn;
  result->data_bytes_written = 0;
  result->error = error;
  result->finished = false;
}

#ifdef __cplusplus
} // extern "C"
#endif
