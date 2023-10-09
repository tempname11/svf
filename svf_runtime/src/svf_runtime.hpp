#ifndef SVF_RUNTIME_HPP
#define SVF_RUNTIME_HPP

#ifdef __cplusplus

#include <cstdint>

#ifndef SVFRT_SINGLE_FILE
  #include "svf_runtime.h"
#endif

namespace svf {

#ifndef SVF_COMMON_CPP_TYPES_INCLUDED
#define SVF_COMMON_CPP_TYPES_INCLUDED
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

#pragma pack(push, 1)

template<typename T>
struct Reference {
  U32 data_offset_complement;
};

template<typename T>
struct Sequence {
  U32 data_offset_complement;
  U32 count;
};

#pragma pack(pop)
#endif // SVF_COMMON_CPP_TYPES_INCLUDED

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

typedef Range<U8> Bytes;

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
  T const *entry;
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
    SVFRT_Bytes {
      /*.pointer =*/ message_bytes.pointer,
      /*.count =*/ message_bytes.count,
    },
    SchemaDescription::template PerType<Entry>::name_hash,
    SchemaDescription::template PerType<Entry>::index,
    SVFRT_Bytes {
      /*.pointer =*/ (U8 *) SchemaDescription::schema_array,
      /*.count =*/ SchemaDescription::schema_size
    },
    SVFRT_Bytes {
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
T const *read_reference(
  ReadContext *ctx,
  Reference<T> reference
) {
  auto data_offset = ~reference.data_offset_complement;
  if (data_offset + sizeof(T) > ctx->data_range.count) {
    return 0;
  }
  return (T *) (ctx->data_range.pointer + data_offset);
}

template<typename T>
static inline
Range<T const> read_sequence_raw(
  ReadContext *ctx,
  Sequence<T> sequence
) {
  // TODO: type must be primitive, check that.

  auto data_offset = ~sequence.data_offset_complement;

  if (data_offset + sequence.count * sizeof(T) > ctx->data_range.count) {
    return {0, 0};
  }
  return { (T *) (ctx->data_range.pointer + data_offset), sequence.count };
}

template<typename T>
static inline
T const *read_sequence_element(
  ReadContext *ctx,
  Sequence<T> sequence,
  U32 index
) {
  using SchemaDescription = typename svf::GetSchemaFromType<T>::SchemaDescription;
  auto stride = ctx->struct_strides.pointer[SchemaDescription::template PerType<T>::index];
  if (index > sequence.count) {
    return 0;
  }
  auto item_offset = ~sequence.data_offset_complement + stride * index;
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
Reference<T> write_reference(
  WriteContext<E> *ctx,
  T const *in_pointer
) {
  auto bytes = SVFRT_Bytes {
    /*.pointer =*/ (U8 *) in_pointer,
    /*.count =*/ sizeof(T),
  };

  // TODO @proper-alignment.
  auto written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
    return {};
  }

  auto result = Reference<T> {
    /*.data_offset_complement =*/ ~ctx->data_bytes_written
  };

  // TODO @correctness: potential int overflow?
  ctx->data_bytes_written += bytes.count;
  return result;
}

template<typename T>
static inline
void write_message_end(
  WriteContext<T> *ctx,
  T const *in_pointer
) {
  write_reference<T, T>(ctx, in_pointer);
  if (!ctx->error) {
    ctx->finished = true;
  }
}

template<typename T, typename E>
static inline
Sequence<T> write_sequence(
  WriteContext<E> *ctx,
  T const *in_pointer,
  U32 size
) {
  auto bytes = SVFRT_Bytes {
    /*.pointer =*/ (U8 *) in_pointer,
    /*.count =*/ sizeof(T) * size,
  };

  // TODO @proper-alignment.
  auto written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
  }

  auto data_offset = ctx->data_bytes_written;
  ctx->data_bytes_written += bytes.count;
  return {
    /*.data_offset_complement =*/ ~data_offset,
    /*.count =*/ size,
  };
}

template<typename T, typename E, int N>
static inline
Sequence<T> write_fixed_size_array(
  WriteContext<E> *ctx,
  T const (&in_array)[N]
) {
  return write_sequence(ctx, in_array, (U32) N);
}

template<typename T, typename E, int N>
static inline
Sequence<U8> write_fixed_size_array_u8(
  WriteContext<E> *ctx,
  T const (&in_array)[N]
) {
  static_assert(sizeof(T) == 1);
  return write_sequence<U8>(ctx, (U8 *) in_array, (U32) N);
}

template<typename T, typename E>
static inline
void write_sequence_element(
  WriteContext<E> *ctx,
  T const *in_pointer,
  Sequence<T> *inout_sequence
) {
  if (
    inout_sequence->count != 0 &&
    ~inout_sequence->data_offset_complement + inout_sequence->count * sizeof(T) != ctx->data_bytes_written
  ) {
    ctx->error = true;
    return;
  }

  auto bytes = SVFRT_Bytes {
    /*.pointer =*/ (U8 *) in_pointer,
    /*.count =*/ sizeof(T),
  };

  // TODO @proper-alignment.
  auto written = ctx->writer_fn(ctx->writer_ptr, bytes);

  if (written != bytes.count) {
    ctx->error = true;
  }

  if (inout_sequence->count == 0) {
    inout_sequence->data_offset_complement = ~ctx->data_bytes_written;
  }
  inout_sequence->count += 1;
  ctx->data_bytes_written += bytes.count;
}

} // namespace runtime
} // namespace svf

#endif // __cplusplus

#endif // SVF_RUNTIME_HPP
