#ifndef SVF_RUNTIME_H
#define SVF_RUNTIME_H

#ifdef __cplusplus
  #include <climits>
  #include <cstddef>
  #include <cstdint>
  #include <cstdbool>
#else
  #include <limits.h>
  #include <stddef.h>
  #include <stdint.h>
  #include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__clang__) && defined(__LITTLE_ENDIAN__)
  #define SVF_PLATFORM_LITTLE_ENDIAN 1
#endif

#ifndef SVF_PLATFORM_LITTLE_ENDIAN
  #error "This library only supports little-endian platforms. Auto-detection failed, so if your platform is indeed little-endian, please #define SVF_PLATFORM_LITTLE_ENDIAN manually."
#endif

#if CHAR_BIT != 8
  #error "This library only supports 8-bit bytes. What on earth are you compiling for?!"
#endif

// TOOD @support: check size_t and uintptr_t (>= 32 bit).

#ifndef SVF_COMMON_C_TYPES_INCLUDED
#define SVF_COMMON_C_TYPES_INCLUDED
#pragma pack(push, 1)

typedef struct SVFRT_Reference {
  uint32_t data_offset_complement;
} SVFRT_Reference;

typedef struct SVFRT_Sequence {
  uint32_t data_offset_complement;
  uint32_t count;
} SVFRT_Sequence;

#pragma pack(pop)
#endif // SVF_COMMON_C_TYPES_INCLUDED

typedef struct SVFRT_Bytes {
  uint8_t *pointer;
  uint32_t count;
} SVFRT_Bytes;

typedef struct SVFRT_RangeU32 {
  uint32_t *pointer;
  uint32_t count;
} SVFRT_RangeU32;

#pragma pack(push, 1)
typedef struct SVFRT_MessageHeader {
  uint8_t magic[3];
  uint8_t version;
  uint32_t schema_length;
  uint64_t entry_name_hash;
} SVFRT_MessageHeader;
#pragma pack(pop)

// Alignment for message parts: header, schema, data.
#define SVFRT_MESSAGE_PART_ALIGNMENT 8

// If tags ever become capable of being > 1 byte wide, this macro needs to be
// removed altogether. Code that relies on it being exactly 1 byte currently,
// needs to reference this macro.
#define SVFRT_TAG_SIZE 1

// By default, fail, if checking an untrusted schema for compatibility is more
// than `N` (this factor) times longer than the base scenario, which is to check
// the schema with itself.
//
// TODO: arbitrary, but hopefully sane choice.
#define SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR 8

#define SVFRT_DEFAULT_MAX_RECURSION_DEPTH 256
#define SVFRT_NO_SIZE_LIMIT UINT32_MAX

// Must allocate with alignment of at least `SVFRT_MESSAGE_PART_ALIGNMENT`.
typedef void *(SVFRT_AllocatorFn)(void *allocate_ptr, size_t size);

typedef struct SVFRT_ReadContext {
  SVFRT_Bytes data_range;
  SVFRT_RangeU32 struct_strides;
} SVFRT_ReadContext;

typedef enum SVFRT_CompatibilityLevel {
  SVFRT_compatibility_none = 0,
  SVFRT_compatibility_logical = 1,
  SVFRT_compatibility_binary = 2,
  SVFRT_compatibility_exact = 3,
} SVFRT_CompatibilityLevel;

typedef struct SVFRT_ReadMessageResult {
  void *entry;
  void *allocation;
  SVFRT_CompatibilityLevel compatibility_level;
  SVFRT_ReadContext context; // Only valid, if `entry` is valid.
} SVFRT_ReadMessageResult;

// Read the message.
// - `scratch_memory` must have a certain size dependent on the read schema.
// See `min_read_scratch_memory_size` in the header generated from the read schema.
void SVFRT_read_message_implementation(
  SVFRT_ReadMessageResult *result,
  SVFRT_Bytes message_bytes,
  uint64_t entry_name_hash,
  uint32_t entry_index,
  SVFRT_Bytes expected_schema,
  uint32_t work_max, // TODO: three `uint32_t` limits in a row, will be easy to mix them up...
  uint32_t max_recursion_depth,
  uint32_t max_output_size,
  SVFRT_Bytes scratch_memory,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_AllocatorFn *allocator_fn, // Only for SVFRT_compatibility_logical.
  void *allocator_ptr              // Only for SVFRT_compatibility_logical.
);

typedef uint32_t SVFRT_ErrorCode;
// TODO: codes are not final, tidy them up (after all or most of them are listed).

#define SVFRT_code_compatibility__work_max_exceeded                   0x00010001
#define SVFRT_code_compatibility__required_level_is_none              0x00010002
#define SVFRT_code_compatibility__invalid_sufficient_level            0x00010003
#define SVFRT_code_compatibility__not_enough_scratch_memory           0x00010004
#define SVFRT_code_compatibility__entry_name_hash_not_found           0x00010005
#define SVFRT_code_compatibility__struct_index_mismatch               0x00010006
#define SVFRT_code_compatibility__invalid_structs                     0x00010007
#define SVFRT_code_compatibility__invalid_choices                     0x00010008
#define SVFRT_code_compatibility__invalid_fields                      0x00010009
#define SVFRT_code_compatibility__invalid_options                     0x0001000A
#define SVFRT_code_compatibility__field_is_missing                    0x0001000B
#define SVFRT_code_compatibility__field_offset_mismatch               0x0001000C
#define SVFRT_code_compatibility__struct_size_mismatch                0x0001000D
#define SVFRT_code_compatibility__choice_index_mismatch               0x0001000E
#define SVFRT_code_compatibility__option_is_missing                   0x0001000F
#define SVFRT_code_compatibility__option_tag_mismatch                 0x00010010
#define SVFRT_code_compatibility__type_mismatch                       0x00010011
#define SVFRT_code_compatibility__concrete_type_mismatch              0x00010012
#define SVFRT_code_compatibility__invalid_struct_index                0x00010013
#define SVFRT_code_compatibility__invalid_choice_index                0x00010014

#define SVFRT_code_compatibility_internal__invalid_type_enum          0x00020001
#define SVFRT_code_compatibility_internal__invalid_structs            0x00020007
#define SVFRT_code_compatibility_internal__invalid_choices            0x00020008
#define SVFRT_code_compatibility_internal__invalid_fields             0x00020009
#define SVFRT_code_compatibility_internal__invalid_options            0x0002000A
#define SVFRT_code_compatibility_internal__queue_overflow             0x0002000B
#define SVFRT_code_compatibility_internal__queue_unhandled            0x0002000C

#define SVFRT_code_conversion__allocation_failed                      0x00030001
#define SVFRT_code_conversion__total_data_size_limit_exceeded         0x00030002
#define SVFRT_code_conversion__bad_schema_structs                     0x00030003
#define SVFRT_code_conversion__bad_schema_choices                     0x00030004
#define SVFRT_code_conversion__bad_schema_fields                      0x00030005
#define SVFRT_code_conversion__bad_schema_options                     0x00030006
#define SVFRT_code_conversion__bad_schema_struct_index                0x00030007
#define SVFRT_code_conversion__bad_schema_field_index                 0x00030008
#define SVFRT_code_conversion__bad_schema_choice_index                0x00030009
#define SVFRT_code_conversion__bad_schema_option_index                0x0003000A
#define SVFRT_code_conversion__bad_schema_type_enum                   0x0003000B
#define SVFRT_code_conversion__data_out_of_bounds                     0x0003000C
#define SVFRT_code_conversion__bad_choice_tag                         0x0003000D
#define SVFRT_code_conversion__schema_type_enum_mismatch              0x0003000E
#define SVFRT_code_conversion__schema_concrete_type_enum_mismatch     0x0003000F
#define SVFRT_code_conversion__max_recursion_depth_exceeded           0x00030010
#define SVFRT_code_conversion__bad_type                               0x00030011
#define SVFRT_code_conversion__data_aliasing_detected                 0x00030012

#define SVFRT_code_conversion_internal__suballocation_mismatch        0x00040001
#define SVFRT_code_conversion_internal__suballocation_failed          0x00040002
#define SVFRT_code_conversion_internal__bad_schema_structs            0x00040003
#define SVFRT_code_conversion_internal__bad_schema_choices            0x00040004
#define SVFRT_code_conversion_internal__bad_schema_fields             0x00040005
#define SVFRT_code_conversion_internal__bad_schema_options            0x00040006
#define SVFRT_code_conversion_internal__bad_schema_struct_index       0x00040007
#define SVFRT_code_conversion_internal__bad_schema_field_index        0x00040008
#define SVFRT_code_conversion_internal__bad_schema_choice_index       0x00040009
#define SVFRT_code_conversion_internal__bad_schema_option_index       0x0004000A
#define SVFRT_code_conversion_internal__schema_field_missing          0x0004000B
#define SVFRT_code_conversion_internal__schema_option_missing         0x0004000C
#define SVFRT_code_conversion_internal__schema_impossible_type        0x0004000D
#define SVFRT_code_conversion_internal__schema_incompatible_types     0x0004000E
#define SVFRT_code_conversion_internal__suballocation_out_of_bounds   0x0004000F
#define SVFRT_code_conversion_internal__bad_type                      0x00040010

typedef struct SVFRT_CompatibilityResult {
  SVFRT_CompatibilityLevel level;
  SVFRT_RangeU32 quirky_struct_strides_dst; // See #logical-compatibility-stride-quirk.
  SVFRT_ErrorCode error_code; // See `SVFRT_code_compatibility__*`.

  // Internal.
  uint32_t unsafe_entry_size_src;
  uint32_t entry_struct_index_src;
  uint32_t entry_struct_index_dst;
} SVFRT_CompatibilityResult;

// Check compatibility of two schemas, i.e. can the data written in one schema
// ("src"), be read using another schema ("dst").
//
// - `result` must be zero-initialized.
// - `scratch_memory` must have a certain size dependent on the read schema.
// See `min_read_scratch_memory_size` in the header generated from the read schema.
//
// TODO: consider, whether this should be internal.
void SVFRT_check_compatibility(
  SVFRT_CompatibilityResult *out_result,
  SVFRT_Bytes scratch_memory,
  SVFRT_Bytes unsafe_schema_src,
  SVFRT_Bytes schema_dst,
  uint64_t entry_name_hash,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_CompatibilityLevel sufficient_level,
  uint32_t work_max
);

typedef uint32_t (SVFRT_WriterFn)(void *write_pointer, SVFRT_Bytes data);

typedef struct SVFRT_WriteContext {
  void *writer_ptr;
  SVFRT_WriterFn *writer_fn;
  uint32_t data_bytes_written;

  // TODO: more granular error reporting.
  bool error;

  bool finished;
} SVFRT_WriteContext;

void SVFRT_write_message_implementation(
  SVFRT_WriteContext *result,
  void *writer_pointer,
  SVFRT_WriterFn *writer_fn,
  SVFRT_Bytes schema_bytes,
  uint64_t entry_name_hash
);

static inline
void SVFRT_internal_write_increment_counter(
  SVFRT_WriteContext *ctx,
  uint32_t written
) {
  ctx->data_bytes_written += written;

  if (ctx->data_bytes_written < written) {
    // We got an overflow.
    ctx->error = true;
  }
}

static inline
SVFRT_Reference SVFRT_write_reference(
  SVFRT_WriteContext *ctx,
  void *pointer,
  uint32_t type_size
) {
  SVFRT_Bytes bytes = { (uint8_t *) pointer, type_size };

  // TODO @proper-alignment.
  uint32_t written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
    SVFRT_Reference result = {0};
    return result;
  }

  SVFRT_Reference result = { ~ctx->data_bytes_written };
  SVFRT_internal_write_increment_counter(ctx, written);
  return result;
}

static inline
SVFRT_Sequence SVFRT_write_sequence(
  SVFRT_WriteContext *ctx,
  void *pointer,
  uint32_t type_size,
  uint32_t count
) {
  // Prevent addition overflow by casting operands to `uint64_t` first.
  uint64_t total_size = (uint64_t) type_size * (uint64_t) count;
  if (total_size > (uint64_t) UINT32_MAX) {
    ctx->error = true;
    SVFRT_Sequence result = {0, 0};
    return result;
  }
  SVFRT_Bytes bytes = { (uint8_t *) pointer, (uint32_t) total_size };

  // TODO @proper-alignment.
  uint32_t written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
    SVFRT_Sequence result = {0, 0};
    return result;
  }

  SVFRT_Sequence result = { ~ctx->data_bytes_written, count };
  SVFRT_internal_write_increment_counter(ctx, written);
  return result;
}

static inline
void SVFRT_write_sequence_element(
  SVFRT_WriteContext *ctx,
  void *pointer,
  uint32_t type_size,
  SVFRT_Sequence *inout_sequence
) {
  // Prevent multiply-add overflow by casting operands to `uint64_t` first. It
  // works, because `UINT64_MAX == UINT32_MAX * UINT32_MAX + UINT32_MAX + UINT32_MAX`.
  uint64_t end_offset = (uint64_t) (~inout_sequence->data_offset_complement) + (
    (uint64_t) type_size * (uint64_t) inout_sequence->count
  );

  // Make sure we write contiguously.
  if (inout_sequence->count != 0 && end_offset != (uint64_t) ctx->data_bytes_written) {
    ctx->error = true;
    return;
  }

  if (inout_sequence->count == UINT32_MAX) {
    ctx->error = true;
    return;
  }

  SVFRT_Bytes bytes = { (uint8_t *) pointer, type_size };

  // TODO @proper-alignment.
  uint32_t written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
    return;
  }

  if (inout_sequence->count == 0) {
    inout_sequence->data_offset_complement = ~ctx->data_bytes_written;
  }

  // Overflow cannot happen because of an earlier check.
  inout_sequence->count += 1;
  SVFRT_internal_write_increment_counter(ctx, written);
}

static inline
void SVFRT_write_message_end(
  SVFRT_WriteContext *ctx,
  void *pointer,
  uint32_t type_size
) {
  SVFRT_write_reference(ctx, pointer, type_size);
  if (!ctx->error) {
    ctx->finished = true;
  }
}

static inline
void const *SVFRT_read_reference(
  SVFRT_ReadContext *ctx,
  SVFRT_Reference reference,
  uint32_t type_size
) {
  uint32_t data_offset = ~reference.data_offset_complement;

  // Prevent addition overflow by casting operands to `uint64_t` first.
  if ((uint64_t) data_offset + (uint64_t) type_size > (uint64_t) ctx->data_range.count) {
    return NULL;
  }

  return (void *) (ctx->data_range.pointer + data_offset);
}

// Warning!
//
// For structs, this is only applicable when you know the stride, which may not
// be equal to the `sizeof` of your type. They may not be equal for
// `SVFRT_compatibility_binary`. However, for they will always be equal for:
// - `SVFRT_compatibility_exact` (since it is exact).
// - `SVFRT_compatibility_logical` (because the conversion makes it exact).
//
// Whenever the stride is not equal to the `sizeof` of your type, you can't use
// pointer arithmetic on `YourType *`, and using this function is not recommended.
// Please use `SVFRT_read_sequence_element` instead in this case.
//
// For primitives, this can be always used.
static inline
void const *SVFRT_read_sequence_raw(
  SVFRT_ReadContext *ctx,
  SVFRT_Sequence sequence,
  uint32_t type_stride
) {
  uint32_t data_offset = ~sequence.data_offset_complement;

  // Prevent multiply-add overflow by casting operands to `uint64_t` first. It
  // works, because `UINT64_MAX == UINT32_MAX * UINT32_MAX + UINT32_MAX + UINT32_MAX`.
  uint64_t end_offset = (uint64_t) data_offset + (
    (uint64_t) sequence.count * (uint64_t) type_stride
  );

  // Check end of the range.
  if (end_offset > (uint64_t) ctx->data_range.count) {
    return NULL;
  }

  return (void *) (ctx->data_range.pointer + data_offset);
}

static inline
void const *SVFRT_read_sequence_element(
  SVFRT_ReadContext *ctx,
  SVFRT_Sequence sequence,
  uint32_t struct_index,
  uint32_t element_index
) {
  // This check is not necessary when using this via the macro, but it is here
  // in case the function is called directly.
  if (struct_index >= ctx->struct_strides.count) {
    return NULL;
  }

  uint32_t stride = ctx->struct_strides.pointer[struct_index];

  // Basic index check, and this also guarantees that `element_index < UINT32_MAX`.
  if (element_index >= sequence.count) {
    return NULL;
  }

  // Prevent multiply-add overflow by casting operands to `uint64_t` first. It
  // works, because `UINT64_MAX == UINT32_MAX * UINT32_MAX + UINT32_MAX + UINT32_MAX`.
  uint64_t item_end_offset = (
    (uint64_t) (~sequence.data_offset_complement) +
    (uint64_t) stride * ((uint64_t) element_index + 1)
  );

  // Check end of the range. This also guarantees that `item_end_offset <= UINT32_MAX`.
  if (item_end_offset > (uint64_t) ctx->data_range.count) {
    return NULL;
  }

  return (void *) (ctx->data_range.pointer + (uint32_t) item_end_offset - stride);
}

#define SVFRT_WRITE_MESSAGE_START(schema_name, entry_name, ctx, writer_fn, writer_ptr) \
  SVFRT_write_message_implementation( \
    (ctx), \
    (writer_ptr), \
    (writer_fn), \
    (SVFRT_Bytes) { (void *) schema_name ## _schema_binary_array, schema_name ## _schema_binary_size }, \
    entry_name ## _name_hash \
  )

#define SVFRT_WRITE_REFERENCE(ctx, data_ptr) \
  SVFRT_write_reference((ctx), (void *) (data_ptr), sizeof(*(data_ptr)))

#define SVFRT_WRITE_FIXED_SIZE_ARRAY(ctx, array) \
  SVFRT_write_sequence((ctx), (void *) (array), sizeof(*(array)), sizeof(array) / sizeof(*(array)))

#define SVFRT_WRITE_SEQUENCE_ELEMENT(ctx, data_ptr, inout_sequence) \
  SVFRT_write_sequence_element((ctx), (void *) (data_ptr), sizeof(*(data_ptr)), (inout_sequence))

#define SVFRT_WRITE_MESSAGE_END(ctx, data_ptr) \
  SVFRT_write_message_end((ctx), (void *) (data_ptr), sizeof(*(data_ptr)))

// `*out_result` must be zero-initialized.
#define SVFRT_READ_MESSAGE(schema_name, entry_name, out_result, message_bytes, scratch_memory, required_level, allocator_fn, allocator_ptr) \
  SVFRT_read_message_implementation( \
    (out_result), \
    (message_bytes), \
    (entry_name ## _name_hash), \
    (entry_name ## _struct_index), \
    (SVFRT_Bytes) { (void *) schema_name ## _schema_binary_array, schema_name ## _schema_binary_size }, \
    (schema_name ## _compatibility_work_base) * SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR, \
    SVFRT_DEFAULT_MAX_RECURSION_DEPTH, \
    SVFRT_NO_SIZE_LIMIT, \
    (scratch_memory), \
    (required_level), \
    (allocator_fn), \
    (allocator_ptr) \
  )

#define SVFRT_READ_REFERENCE(type_name, ctx, reference) \
  ((type_name const *) SVFRT_read_reference((ctx), (reference), sizeof(type_name)))

#define SVFRT_READ_SEQUENCE_ELEMENT(type_name, ctx, sequence, element_index) \
  ((type_name const *) SVFRT_read_sequence_element((ctx), (sequence), type_name ## _struct_index, element_index))

// Warning! See `SVFRT_read_sequence_raw` for caveats.
#define SVFRT_READ_SEQUENCE_RAW(type_name, ctx, sequence) \
  ((type_name const *) SVFRT_read_sequence_raw((ctx), (sequence), sizeof(type_name)))

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SVF_RUNTIME_H
