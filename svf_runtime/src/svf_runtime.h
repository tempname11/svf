#ifndef SVF_RUNTIME_H
#define SVF_RUNTIME_H

#ifdef __cplusplus
  #include <cstddef>
  #include <cstdint>
  #include <cstdbool>
#else
  #include <stddef.h>
  #include <stdint.h>
  #include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SVF_COMMON_C_TYPES_INCLUDED
#define SVF_COMMON_C_TYPES_INCLUDED
#pragma pack(push, 1)

typedef struct SVFRT_Pointer {
  uint32_t data_offset;
} SVFRT_Pointer;

typedef struct SVFRT_Array {
  uint32_t data_offset;
  uint32_t count;
} SVFRT_Array;

#pragma pack(pop)
#endif // SVF_COMMON_C_TYPES_INCLUDED

typedef struct SVFRT_Bytes {
  uint8_t *pointer;
  uint64_t count;
} SVFRT_Bytes;

typedef struct SVFRT_RangeU32 {
  uint32_t *pointer;
  uint64_t count;
} SVFRT_RangeU32;

typedef struct SVFRT_MessageHeader {
  uint8_t magic[3];
  uint8_t version;
  uint32_t schema_length;
  uint64_t entry_name_hash;
} SVFRT_MessageHeader;

#define SVFRT_MESSAGE_PART_ALIGNMENT 8

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
  SVFRT_ReadContext context;
} SVFRT_ReadMessageResult;

void SVFRT_read_message_implementation(
  SVFRT_ReadMessageResult *result,
  SVFRT_Bytes message_bytes,
  uint64_t entry_name_hash,
  uint32_t entry_index,
  SVFRT_Bytes expected_schema,
  SVFRT_Bytes scratch_memory,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_AllocatorFn *allocator_fn, // Only for SVFRT_compatibility_logical
  void *allocator_ptr              // Only for SVFRT_compatibility_logical
);

typedef struct SVFRT_CompatibilityResult {
  SVFRT_CompatibilityLevel level;
  SVFRT_RangeU32 struct_strides;

  // Internal.
  uint32_t entry_size0;
  uint32_t entry_struct_index0;
  uint32_t entry_struct_index1;
} SVFRT_CompatibilityResult;

// Check compatibility of two schemas.
// `result` must be zero-filled.
// `scratch_memory` must have a certain size dependent on the schema.
void SVFRT_check_compatibility(
  SVFRT_CompatibilityResult *result,
  SVFRT_Bytes scratch_memory,
  SVFRT_Bytes schema_write,
  SVFRT_Bytes schema_read,
  uint64_t entry_name_hash,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_CompatibilityLevel sufficient_level
);

typedef uint32_t (SVFRT_WriterFn)(void *write_pointer, SVFRT_Bytes data);

typedef struct SVFRT_WriteContext {
  void *writer_ptr;
  SVFRT_WriterFn *writer_fn;
  uint32_t data_bytes_written;
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

// Start of a more experimental C (& preprocessor) API. Mostly follows
// "runtime.hpp" for now. It should probably be changed a bit to prevent usage
// errors.

static inline
SVFRT_Pointer SVFRT_write_pointer(
  SVFRT_WriteContext *ctx,
  void *in_pointer,
  size_t size
) {
  SVFRT_Bytes bytes = { (uint8_t *) in_pointer, size };

  // Will break on @proper-alignment.
  uint32_t written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
    SVFRT_Pointer result = {0};
    return result;
  }

  SVFRT_Pointer result = { ctx->data_bytes_written };

  // TODO: Potential overflow...
  ctx->data_bytes_written += bytes.count;
  return result;
}

static inline
SVFRT_Array SVFRT_write_array(
  SVFRT_WriteContext *ctx,
  void *in_pointer,
  size_t size,
  uint32_t count
) {
  SVFRT_Bytes bytes = { (uint8_t *) in_pointer, size * count };

  // Will break on @proper-alignment.
  uint32_t written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
    SVFRT_Array result = {0, 0};
    return result;
  }

  SVFRT_Array result = { ctx->data_bytes_written, count };

  // TODO: Potential overflow...
  ctx->data_bytes_written += bytes.count;
  return result;
}

static inline
void SVFRT_write_array_element(
  SVFRT_WriteContext *ctx,
  void *in_pointer,
  size_t size,
  SVFRT_Array *in_array
) {
  if (
    in_array->count != 0 &&
    in_array->data_offset + size != ctx->data_bytes_written
  ) {
    ctx->error = true;
    return;
  }

  SVFRT_Bytes bytes = { (uint8_t *) in_pointer, size };

  // Will break on @proper-alignment.
  uint32_t written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
  }

  if (in_array->count == 0) {
    in_array->data_offset = ctx->data_bytes_written;
  }

  // TODO: Potential overflow...
  in_array->count += 1;
  ctx->data_bytes_written += bytes.count;
}

static inline
void SVFRT_write_message_end(
  SVFRT_WriteContext *ctx,
  void *in_pointer,
  size_t size
) {
  SVFRT_write_pointer(ctx, in_pointer, size);
  if (!ctx->error) {
    ctx->finished = true;
  }
}

static inline
void const *SVFRT_read_pointer(
  SVFRT_ReadContext *ctx,
  SVFRT_Pointer pointer,
  size_t size
) {
  if (pointer.data_offset + size > ctx->data_range.count) {
    return NULL;
  }
  return (void *) (ctx->data_range.pointer + pointer.data_offset);
}

static inline
void const *SVFRT_read_array_raw(
  SVFRT_ReadContext *ctx,
  SVFRT_Array array,
  size_t size
) {
  if (array.data_offset + size * array.count > ctx->data_range.count) {
    return NULL;
  }
  return (void *) (ctx->data_range.pointer + array.data_offset);
}

static inline
void const *SVFRT_read_array_element(
  SVFRT_ReadContext *ctx,
  SVFRT_Array array,
  size_t size,
  uint32_t struct_index,
  uint32_t element_index
) {
  uint32_t stride = ctx->struct_strides.pointer[struct_index];
  if (element_index > array.count) {
    return NULL;
  }
 uint32_t item_offset = array.data_offset + stride * element_index;
  if (item_offset + size > ctx->data_range.count) {
    return NULL;
  }
  return (void *) (ctx->data_range.pointer + item_offset);
}

#define SVFRT_WRITE_MESSAGE_START(schema_name, entry_name, ctx, writer_fn, writer_ptr) \
  SVFRT_write_message_implementation( \
    (ctx), \
    (writer_ptr), \
    (writer_fn), \
    (SVFRT_Bytes) { (void *) schema_name ## _binary_array, schema_name ## _binary_size }, \
    entry_name ## _name_hash \
  )

#define SVFRT_WRITE_POINTER(ctx, data_ptr) \
  SVFRT_write_pointer((ctx), (void *) (data_ptr), sizeof(*(data_ptr)))

#define SVFRT_WRITE_FIXED_SIZE_ARRAY(ctx, array) \
  SVFRT_write_array((ctx), (void *) (array), sizeof(*(array)), sizeof(array) / sizeof(*(array)))

#define SVFRT_WRITE_ARRAY_ELEMENT(ctx, data_ptr, array_ptr) \
  SVFRT_write_array_element((ctx), (void *) (data_ptr), sizeof(*(data_ptr)), (array_ptr))

#define SVFRT_WRITE_MESSAGE_END(ctx, data_ptr) \
  SVFRT_write_message_end((ctx), (void *) (data_ptr), sizeof(*(data_ptr)))

#define SVFRT_READ_MESSAGE(schema_name, entry_name, out_ptr, message_bytes, scratch_memory, required_level, allocator_fn, allocator_ptr) \
  SVFRT_read_message_implementation( \
    (out_ptr), \
    (message_bytes), \
    entry_name ## _name_hash, \
    entry_name ## _struct_index, \
    (SVFRT_Bytes) { (void *) schema_name ## _binary_array, schema_name ## _binary_size }, \
    (scratch_memory), \
    (required_level), \
    (allocator_fn), \
    (allocator_ptr) \
  )

#define SVFRT_READ_POINTER(type_name, ctx, pointer) \
  ((type_name const *) SVFRT_read_pointer((ctx), (pointer), sizeof(type_name)))

#define SVFRT_READ_ARRAY_ELEMENT(type_name, ctx, array, element_index) \
  ((type_name const *) SVFRT_read_array_element((ctx), (array), sizeof(type_name), type_name ## _struct_index, element_index))

#define SVFRT_READ_ARRAY_RAW(type_name, ctx, array) \
  ((type_name const *) SVFRT_read_array_raw((ctx), (array), sizeof(type_name)))

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SVF_RUNTIME_H
