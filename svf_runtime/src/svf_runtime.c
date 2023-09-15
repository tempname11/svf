#include <assert.h>

#include "svf_internal.h"
#include "svf_runtime.h"
#include "svf_meta.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline
size_t align_down(size_t value, size_t alignment) {
  return value - value % alignment;
}

static inline
size_t align_up(size_t value, size_t alignment) {
  // Sanity check to prevent overflow.
  assert(value + alignment > value);

  return value + alignment - 1 - (value - 1) % alignment;
}

#define SVFRT_MAX_CONVERSION_RECURSION_DEPTH 256 // Some hopefully sane default.

void SVFRT_read_message_implementation(
  SVFRT_ReadMessageResult *result,
  SVFRT_Bytes message_bytes,
  uint64_t entry_name_hash,
  uint32_t entry_index,
  SVFRT_Bytes expected_schema,
  SVFRT_Bytes scratch_memory,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_AllocatorFn *allocator_fn,
  void *allocator_ptr
) {
  if (message_bytes.count < sizeof(SVFRT_MessageHeader)) {
    // printf("Too small for an SVF header.\n");
    return;
  }

  // Maybe not needed for all cases?
  if (((size_t) message_bytes.pointer) % SVFRT_MESSAGE_PART_ALIGNMENT != 0) {
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

  // For now, versions must match exactly.
  if (header->version != 0) {
    // printf("Does not match SVF version 0.\n");
    return;
  }

  // Make sure the declared entry is the same as we expect.
  if (header->entry_name_hash != entry_name_hash) {
    // printf("Does not match SVF version 0.\n");
    return;
  }

  // Make sure the schema is in-bounds.
  if (sizeof(SVFRT_MessageHeader) + (size_t) (header->schema_length) > message_bytes.count) {
    // printf("Schema is not in-bounds.\n");
    return;
  }

  // Make sure we can read the schema.
  if (header->schema_length < sizeof(SVF_META_Schema)) {
    // printf("Schema is too small.\n");
    return;
  }

  // We now have a valid schema range.
  SVFRT_Bytes schema_range = {
    /*.pointer =*/ message_bytes.pointer + sizeof(SVFRT_MessageHeader),
    /*.count =*/ header->schema_length,
  };

  uint8_t *data_pointer = (uint8_t *) align_up(
    (size_t) (schema_range.pointer + schema_range.count),
    SVFRT_MESSAGE_PART_ALIGNMENT
  );

  if (data_pointer >= message_bytes.pointer + message_bytes.count) {
    // printf("No data.\n");
    return;
  }

  // We now have a valid data range.
  SVFRT_Bytes data_range = {
    /*.pointer =*/ data_pointer,
    /*.count =*/ (size_t) (
      (message_bytes.pointer + message_bytes.count) - data_pointer
    ),
  };

  SVFRT_CompatibilityResult check_result = {0};
  SVFRT_check_compatibility(
    &check_result,
    scratch_memory,
    schema_range,
    expected_schema,
    entry_name_hash,
    required_level,
    SVFRT_compatibility_exact
  );

  if (check_result.level == SVFRT_compatibility_none) {
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

    SVFRT_Bytes r0 = schema_range;
    SVFRT_Bytes r1 = expected_schema;

    // This will break when proper alignment is done. @proper-alignment
    SVF_META_Schema *s0 = (SVF_META_Schema *) (r0.pointer + r0.count - sizeof(SVF_META_Schema));
    SVF_META_Schema *s1 = (SVF_META_Schema *) (r1.pointer + r1.count - sizeof(SVF_META_Schema));

    SVFRT_ConversionInfo conversion_info = {
      .r0 = r0,
      .r1 = r1,
      .s0 = s0,
      .s1 = s1,
      .struct_index0 = check_result.entry_struct_index0,
      .struct_index1 = check_result.entry_struct_index1,
    };

    uint32_t entry_size = check_result.entry_size0;

    // This will break when proper alignment is done. @proper-alignment
    SVFRT_Bytes entry_input_bytes = {
      /*.pointer =*/ data_range.pointer + data_range.count - entry_size,
      /*.count =*/ entry_size,
    };

    SVFRT_convert_message(
      &conversion_result,
      &conversion_info,
      entry_input_bytes,
      data_range,
      SVFRT_MAX_CONVERSION_RECURSION_DEPTH,
      allocator_fn,
      allocator_ptr
    );

    if (!conversion_result.success) {
      // printf("Conversion was not successful.");
      return;
    }

    final_data_range = conversion_result.output_bytes;
  }

  uint32_t entry_size = check_result.struct_strides.pointer[entry_index];
  size_t entry_alignment = 1; // Will break on @proper-alignment

  if (final_data_range.count < entry_size) {
    // printf("Data is too small.\n");
    return;
  }

  // This will break when proper alignment is done. @proper-alignment
  result->compatibility_level = check_result.level;
  if (check_result.level == SVFRT_compatibility_logical) {
    result->allocation = final_data_range.pointer;
  }
  result->entry = (void *) align_down(
    (size_t) final_data_range.pointer + final_data_range.count - entry_size,
    entry_alignment
  );
  result->context.data_range = final_data_range;
  result->context.struct_strides = check_result.struct_strides;
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
  uint32_t bytes_written = writer_fn(writer_ptr, header_bytes);
  if (bytes_written != header_bytes.count) {
    error = true;
  } else {
    uint32_t written = writer_fn(writer_ptr, schema_bytes);
    if (written != schema_bytes.count) {
      error = true;
    } else {
      bytes_written += written;
    }
  }

  uint8_t zeroes[SVFRT_MESSAGE_PART_ALIGNMENT] = {0};
  size_t misaligned = bytes_written % SVFRT_MESSAGE_PART_ALIGNMENT;
  SVFRT_Bytes padding_range = {
    /*.pointer =*/ (uint8_t *) zeroes,
    /*.count =*/ SVFRT_MESSAGE_PART_ALIGNMENT - misaligned,
  };
  if (misaligned != 0) {
    uint32_t written = writer_fn(writer_ptr, padding_range);
    if (written != padding_range.count) {
      error = true;
    }
  }

  result->writer_ptr = writer_ptr;
  result->writer_fn = writer_fn;
  result->data_bytes_written = 0;
  result->error = error;
}

#ifdef __cplusplus
} // extern "C"
#endif
