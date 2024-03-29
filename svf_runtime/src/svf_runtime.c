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

void SVFRT_read_message(
  SVFRT_ReadMessageParams *params,
  SVFRT_ReadMessageResult *out_result,
  SVFRT_Bytes message,
  SVFRT_Bytes scratch
) {
  out_result->error_code = 0;
  out_result->entry = NULL;
  out_result->allocation = NULL;
  out_result->compatibility_level = SVFRT_compatibility_none;

  if (params->required_level == SVFRT_compatibility_logical && !params->allocator_fn) {
    out_result->error_code = SVFRT_code_read__no_allocator_function;
    return;
  }

  if (message.count < sizeof(SVFRT_MessageHeader)) {
    out_result->error_code = SVFRT_code_read__header_too_small;
    return;
  }

  if (((uintptr_t) message.pointer) % SVFRT_MESSAGE_PART_ALIGNMENT != 0) {
    out_result->error_code = SVFRT_code_read__header_not_aligned;
    return;
  }

  SVFRT_MessageHeader *header = (SVFRT_MessageHeader *) message.pointer;
  if (0
    || header->magic[0] != 'S'
    || header->magic[1] != 'V'
    || header->magic[2] != 'F'
  ) {
    out_result->error_code = SVFRT_code_read__header_magic_mismatch;
    return;
  }

  // For now, versions must match exactly. Version 0 is for development only and
  // does not come with any guarantees.
  if (header->version != 0) {
    out_result->error_code = SVFRT_code_read__header_version_mismatch;
    return;
  }

  // Make sure the declared entry is the same as we expect.
  if (header->entry_struct_id != params->entry_struct_id) {
    out_result->error_code = SVFRT_code_read__entry_struct_id_mismatch;
    return;
  }

  // Prevent addition overflow by casting operands to `uint64_t` first.
  uint64_t appendix_padded_end_offset = SVFRT_align_up(
    SVFRT_align_up(
      (uint64_t) sizeof(SVFRT_MessageHeader) + (uint64_t) (header->schema_length),
      SVFRT_MESSAGE_PART_ALIGNMENT
    ) + (uint64_t) (header->appendix_length),
    SVFRT_MESSAGE_PART_ALIGNMENT
  );

  // Make sure everything is in-bounds.
  if (appendix_padded_end_offset > (uint64_t) message.count) {
    out_result->error_code = SVFRT_code_read__bad_schema_length;
    return;
  }

  // We now have a valid schema and data ranges. The data range is implicit,
  // from the padded end of the appendix, to the end of the message.
  SVFRT_Bytes schema_range = {
    /*.pointer =*/ message.pointer + sizeof(SVFRT_MessageHeader),
    /*.count =*/ header->schema_length,
  };
  SVFRT_Bytes data_range = {
    /*.pointer =*/ message.pointer + appendix_padded_end_offset,
    /*.count =*/ message.count - (uint32_t) appendix_padded_end_offset,
  };

  if (schema_range.count == 0) {
    // Only a reference to the schema is available, so we have to rely on the
    // user-provided lookup function.

    if (!params->schema_lookup_fn) {
      out_result->error_code = SVFRT_code_read__no_schema_lookup_function;
      return;
    }

    schema_range = params->schema_lookup_fn(params->schema_lookup_ptr, header->schema_content_hash);

    if (!schema_range.pointer) {
      out_result->error_code = SVFRT_code_read__schema_lookup_failed;
      return;
    }
  }

  SVFRT_CompatibilityResult check_result = {0};

  if (params->expected_schema_content_hash == header->schema_content_hash) {
    // Quick path.
    check_result.level = SVFRT_compatibility_exact;
    check_result.quirky_struct_strides_dst = params->expected_schema_struct_strides;
  } else {
    // Slow path.
    SVFRT_check_compatibility(
      &check_result,
      scratch,
      schema_range,
      params->expected_schema,
      params->entry_struct_id,
      params->required_level,
      SVFRT_compatibility_exact, // `sufficient_level`.
      params->max_schema_work
    );
  }

  // Set this here in case of early exits.
  out_result->compatibility_level = check_result.level;

  if (check_result.error_code != 0) {
    out_result->error_code = check_result.error_code;
    return;
  }

  if (check_result.level == 0) {
    // No compatibility, but `error_code` was not set, which should not happen.
    out_result->error_code = SVFRT_code_compatibility_internal__unknown;
    return;
  }

  SVFRT_Bytes final_data_range = data_range;
  if (check_result.level == SVFRT_compatibility_logical) {
    // We have to convert the message.

    // `params->allocator_fn` has already been checked to be non-null, see
    // `SVFRT_code_read__no_allocator_function`.

    SVFRT_ConversionResult conversion_result = {0};
    SVFRT_convert_message(
      &conversion_result,
      &check_result,
      data_range,
      params->max_recursion_depth,
      params->max_output_size,
      params->allocator_fn,
      params->allocator_ptr
    );

    if (!conversion_result.success) {
      out_result->allocation = conversion_result.output_bytes.pointer;
      out_result->error_code = conversion_result.error_code;
      return;
    }

    final_data_range = conversion_result.output_bytes;
  }

  // For at least binary compatibility, this will be the size of the entry in
  // the dst-schema. However, for logical compatibility, this will be the size
  // of the entry in the src-schema.
  //
  // This is non-obvious. See #logical-compatibility-stride-quirk.
  uint32_t entry_size = check_result.quirky_struct_strides_dst.pointer[params->entry_struct_index];

  if (final_data_range.count < entry_size) {
    out_result->error_code = SVFRT_code_read__data_too_small;
    return;
  }

  // TODO @proper-alignment: struct access.
  uint32_t entry_alignment = 1;

  uint32_t final_entry_offset = (uint32_t) SVFRT_align_down(
    final_data_range.count - entry_size,
    entry_alignment
  );

  out_result->entry = (void *) (final_data_range.pointer + final_entry_offset);

  if (check_result.level == SVFRT_compatibility_logical) {
    out_result->allocation = final_data_range.pointer;
  }

  // `out_result->compatibility_level` is already set above.

  out_result->context.data_range = final_data_range;
  out_result->context.struct_strides = check_result.quirky_struct_strides_dst;
}

SVFRT_ErrorCode SVFRT_write_part_padding(
  SVFRT_WriterFn *writer_fn,
  void *writer_ptr,
  uint32_t written_part
) {
  uint8_t zeros[SVFRT_MESSAGE_PART_ALIGNMENT] = {0};
  uint32_t misaligned = written_part % SVFRT_MESSAGE_PART_ALIGNMENT;
  SVFRT_Bytes padding_bytes = {
    /*.pointer =*/ (uint8_t *) zeros,
    /*.count =*/ SVFRT_MESSAGE_PART_ALIGNMENT - misaligned,
  };
  if (misaligned != 0) {
    uint32_t written_padding = writer_fn(writer_ptr, padding_bytes);
    if (written_padding != padding_bytes.count) {
      return SVFRT_code_write__writer_function_failed;
    }
  }

  return 0;
}

void SVFRT_write_start(
  SVFRT_WriteContext *result,
  SVFRT_WriterFn *writer_fn,
  void *writer_ptr,
  uint64_t schema_content_hash,
  SVFRT_Bytes schema_bytes,
  SVFRT_Bytes appendix_bytes,
  uint64_t entry_struct_id
) {
  SVFRT_MessageHeader header = {
    /*.magic =*/ { 'S', 'V', 'F' },
    /*.version =*/ 0,
    /*._reserved =*/ {0},
    /*.schema_length =*/ schema_bytes.count,
    /*.appendix_length =*/ appendix_bytes.count,
    /*.schema_content_hash =*/ schema_content_hash,
    /*.entry_struct_id =*/ entry_struct_id
  };

  SVFRT_Bytes header_bytes = {
    /*.pointer =*/ (uint8_t *) &header,
    /*.count =*/ sizeof(header)
  };

  SVFRT_ErrorCode error_code = 0;

  uint32_t written_header = writer_fn(writer_ptr, header_bytes);
  if (written_header != header_bytes.count) {
    error_code = SVFRT_code_write__writer_function_failed;
  }

  if (error_code == 0 && schema_bytes.count > 0) {
    uint32_t written_schema = writer_fn(writer_ptr, schema_bytes);
    if (written_schema != schema_bytes.count) {
      error_code = SVFRT_code_write__writer_function_failed;
    } else {
      error_code = SVFRT_write_part_padding(writer_fn, writer_ptr, written_schema);
    }
  }

  if (error_code == 0 && appendix_bytes.count > 0) {
    uint32_t written_appendix = writer_fn(writer_ptr, appendix_bytes);
    if (written_appendix != appendix_bytes.count) {
      error_code = SVFRT_code_write__writer_function_failed;
    } else {
      error_code = SVFRT_write_part_padding(writer_fn, writer_ptr, written_appendix);
    }
  }

  result->error_code = error_code;
  result->finished = false;
  result->writer_ptr = writer_ptr;
  result->writer_fn = writer_fn;
  result->data_bytes_written = 0;
}

#ifdef __cplusplus
} // extern "C"
#endif
