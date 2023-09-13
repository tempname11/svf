#pragma once
#include <cstdint>
#include "svf_runtime.h"

// C++ runtime, i.e. code that the end C++ program will include and link, in
// order to work with SVF. Probably should be packaged as a single-file lib,
// but for now, it is not.

namespace svf {

#ifndef SVF_COMMON_TYPES_INCLUDED
#define SVF_COMMON_TYPES_INCLUDED
using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

using I8 = int8_t;
using I16 = int16_t;
using I32 = int32_t;
using I64 = int64_t;

using F32 = float;
using F64 = double;

template<typename T>
struct Pointer {
  U32 data_offset;
};

template<typename T>
struct Array {
  U32 data_offset;
  U32 count;
};
#endif // SVF_COMMON_TYPES_INCLUDED

#ifndef SVF_COMMON_CPP_TRICKERY_INCLUDED
#define SVF_COMMON_CPP_TRICKERY_INCLUDED

template<typename T>
struct GetSchemaFromType;

#endif // SVF_COMMON_CPP_TRICKERY_INCLUDED

namespace runtime {

typedef SVFRT_MessageHeader MessageHeader;

constexpr size_t MESSAGE_PART_ALIGNMENT = SVFRT_MESSAGE_PART_ALIGNMENT;
static_assert(sizeof(MessageHeader) % MESSAGE_PART_ALIGNMENT == 0);

template<typename T>
struct Range {
  T *pointer;
  U64 count;
};

typedef SVFRT_ReadContext ReadContext;
typedef SVFRT_AllocatorFn AllocatorFn;

enum class CompatibilityLevel {
  compatibility_none = SVFRT_compatibility_none,
  compatibility_logical = SVFRT_compatibility_logical,
  compatibility_binary = SVFRT_compatibility_binary,
  compatibility_exact = SVFRT_compatibility_exact,
};

template<typename T>
struct ReadMessageResult {
  T *entry;
  void *allocation;
  CompatibilityLevel compatibility_level;
  ReadContext context;
};

template<typename Entry>
static inline
ReadMessageResult<Entry> read_message(
  Range<U8> message_bytes,
  Range<U8> scratch_memory,
  CompatibilityLevel required_level,
  AllocatorFn *allocator_fn = 0,
  void *allocator_ptr = 0
) {
  using SchemaDescription = typename svf::GetSchemaFromType<Entry>::SchemaDescription;
  SVFRT_ReadMessageResult result = {};
  SVFRT_read_message_implementation(
    &result,
    SVFRT_RangeU8 {
      /*.pointer =*/ message_bytes.pointer,
      /*.count =*/ message_bytes.count,
    },
    SchemaDescription::template PerType<Entry>::name_hash,
    SchemaDescription::template PerType<Entry>::index,
    SVFRT_RangeU8 {
      /*.pointer =*/ (U8 *) SchemaDescription::schema_array,
      /*.count =*/ SchemaDescription::schema_size
    },
    SVFRT_RangeU8 {
      /*.pointer =*/ scratch_memory.pointer,
      /*.count =*/ scratch_memory.count,
    },
    (SVFRT_CompatibilityLevel) required_level,
    allocator_fn,
    allocator_ptr
  );
  return ReadMessageResult<Entry> {
    .entry = (Entry *) result.entry,
    .allocation = result.allocation,
    .compatibility_level = (CompatibilityLevel) result.compatibility_level,
    .context = result.context,
  };
}

template<typename T>
static inline
T *read_pointer(
  ReadContext *ctx,
  Pointer<T> pointer
) {
  if (pointer.data_offset + sizeof(T) > ctx->data_range.count) {
    return 0;
  }
  return (T *) (ctx->data_range.pointer + pointer.data_offset);
}

template<typename T>
static inline
T *read_array(
  ReadContext *ctx,
  Array<T> array,
  U32 index
) {
  using SchemaDescription = typename svf::GetSchemaFromType<T>::SchemaDescription;
  auto stride = ctx->struct_strides.pointer[SchemaDescription::template PerType<T>::index];
  if (index > array.count) {
    return 0;
  }
  auto item_offset = array.data_offset + stride * index;
  if (item_offset + sizeof(T) > ctx->data_range.count) {
    return 0;
  }
  return (T *) (ctx->data_range.pointer + item_offset);
}

typedef SVFRT_WriterFn WriterFn;

template<typename T>
struct WriteContext {
  void *writer_ptr;
  WriterFn *writer_fn;
  U32 data_bytes_written;
  bool error;
  bool finished;
};

template<typename Entry>
static inline
WriteContext<Entry> write_message_start(
  void *writer_ptr,
  WriterFn *writer_fn
) {
  using SchemaDescription = typename svf::GetSchemaFromType<Entry>::SchemaDescription;
  SVFRT_WriteContext context = {};
  SVFRT_write_message_implementation(
    &context,
    writer_ptr,
    writer_fn,
    { SchemaDescription::schema_array, SchemaDescription::schema_size },
    SchemaDescription::template PerType<Entry>::name_hash
  );
  return WriteContext<Entry> {
    .writer_ptr = writer_ptr,
    .writer_fn = writer_fn,
    .data_bytes_written = context.data_bytes_written,
    .error = context.error,
    .finished = context.finished,
  };
}

template<typename T, typename E>
static inline
Pointer<T> write_pointer(
  WriteContext<E> *ctx,
  T *in_pointer
) {
  auto bytes = SVFRT_RangeU8 {
    /*.pointer =*/ (U8 *) in_pointer,
    /*.count =*/ sizeof(T),
  };

  // Will break on @proper-alignment.
  auto written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
    return {};
  }

  auto result = Pointer<T> { ctx->data_bytes_written };
  // Potential overflow...
  ctx->data_bytes_written += bytes.count;
  return result;
}

template<typename T>
static inline
void write_message_end(
  WriteContext<T> *ctx,
  T *in_pointer
) {
  write_pointer<T, T>(ctx, in_pointer);
  if (!ctx->error) {
    ctx->finished = true;
  }
}

template<typename T, typename E>
static inline
void write_array_element(
  WriteContext<E> *ctx,
  T *in_pointer,
  Array<T> *in_array
) {
  if (
    in_array->count != 0 &&
    in_array->data_offset + sizeof(T) != ctx->data_bytes_written
  ) {
    ctx->error = true;
    return;
  }

  auto bytes = SVFRT_RangeU8 {
    /*.pointer =*/(U8 *) in_pointer,
    /*.count =*/ sizeof(T),
  };

  // Will break on @proper-alignment.
  auto written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
  }

  if (in_array->count == 0) {
    in_array->data_offset = ctx->data_bytes_written;
  }
  in_array->count += 1;
  ctx->data_bytes_written += bytes.count;
}

} // namespace runtime
} // namespace svf
