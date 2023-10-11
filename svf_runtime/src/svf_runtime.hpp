#ifndef SVF_RUNTIME_HPP
#define SVF_RUNTIME_HPP

#ifdef __cplusplus

#include <cstdint>

#ifndef SVFRT_SINGLE_FILE
  #include "svf_runtime.h"
#endif

// Note: this file is intended to be compiled as C++11 or later.
//
// TODO @support: check if all popular compilers can actually build it in C++11 mode.

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
template<typename T> struct GetSchemaFromType;
#endif // SVF_COMMON_CPP_TRICKERY_INCLUDED

template<typename T> struct IsPrimitive { using No = char; };
template<> struct IsPrimitive<U8> { using Yes = char; };
template<> struct IsPrimitive<U16> { using Yes = char; };
template<> struct IsPrimitive<U32> { using Yes = char; };
template<> struct IsPrimitive<U64> { using Yes = char; };
template<> struct IsPrimitive<I8> { using Yes = char; };
template<> struct IsPrimitive<I16> { using Yes = char; };
template<> struct IsPrimitive<I32> { using Yes = char; };
template<> struct IsPrimitive<I64> { using Yes = char; };
template<> struct IsPrimitive<F32> { using Yes = char; };
template<> struct IsPrimitive<F64> { using Yes = char; };

namespace runtime {

constexpr size_t MESSAGE_PART_ALIGNMENT = SVFRT_MESSAGE_PART_ALIGNMENT;
typedef SVFRT_MessageHeader MessageHeader;
static_assert(sizeof(MessageHeader) % MESSAGE_PART_ALIGNMENT == 0);

template<typename T> struct Range {
  T *pointer;
  U32 count;
};

typedef Range<U8> Bytes;
typedef SVFRT_ReadContext ReadContext;
typedef SVFRT_AllocatorFn AllocatorFn;
typedef SVFRT_WriterFn WriterFn;

enum class CompatibilityLevel {
  compatibility_none = SVFRT_compatibility_none,
  compatibility_logical = SVFRT_compatibility_logical,
  compatibility_binary = SVFRT_compatibility_binary,
  compatibility_exact = SVFRT_compatibility_exact,
};

// Can't use SVFRT_ReadMessageResult directly because of slightly different
// types, so on any changes, make sure to edit them in sync.
template<typename T>
struct ReadMessageResult {
  T const *entry;
  void *allocation;
  CompatibilityLevel compatibility_level;
  ReadContext context;
};

template<typename T> struct WriteContext: SVFRT_WriteContext {};

template<typename Entry>
static inline
ReadMessageResult<Entry> read_message(
  Range<U8> message_bytes,
  Range<U8> scratch_memory,
  CompatibilityLevel required_level,
  AllocatorFn *allocator_fn = 0,
  void *allocator_ptr = 0
) noexcept {
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
      /*.pointer =*/ (U8 *) SchemaDescription::schema_binary_array,
      /*.count =*/ SchemaDescription::schema_binary_size
    },
    SchemaDescription::compatibility_work_base * SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR,
    SVFRT_DEFAULT_MAX_RECURSION_DEPTH,
    SVFRT_NO_SIZE_LIMIT,
    SVFRT_Bytes {
      /*.pointer =*/ scratch_memory.pointer,
      /*.count =*/ scratch_memory.count,
    },
    (SVFRT_CompatibilityLevel) required_level,
    allocator_fn,
    allocator_ptr
  );
  return ReadMessageResult<Entry> {
    /*.entry =*/ (Entry *) result.entry,
    /*.allocation =*/ result.allocation,
    /*.compatibility_level =*/ (CompatibilityLevel) result.compatibility_level,
    /*.context =*/ result.context,
  };
}

template<typename T>
static inline
T const *read_reference(
  ReadContext *ctx,
  Reference<T> reference
) noexcept {
  return (T *) SVFRT_read_reference(
    ctx,
    SVFRT_Reference { reference.data_offset_complement },
    sizeof(T)
  );
}

template<typename T>
static inline
Range<T const> read_sequence_raw(
  ReadContext *ctx,
  Sequence<T> sequence
) noexcept {
  // `T` must be primitive, see caveats for `SVFRT_read_sequence_raw`.
  // For most other purposes, use `read_sequence_element`.
  static_assert(sizeof(typename IsPrimitive<T>::Yes) > 0);

  auto pointer = SVFRT_read_sequence_raw(
    ctx,
    SVFRT_Sequence { sequence.data_offset_complement, sequence.count },
    sizeof(T)
  );
  return { (T const *) pointer, pointer ? sequence.count : 0 };
}

template<typename T>
static inline
T const *read_sequence_element(
  ReadContext *ctx,
  Sequence<T> sequence,
  U32 element_index
) noexcept {
  using SchemaDescription = typename svf::GetSchemaFromType<T>::SchemaDescription;
  return (T const *) SVFRT_read_sequence_element(
    ctx,
    SVFRT_Sequence { sequence.data_offset_complement, sequence.count },
    SchemaDescription::template PerType<T>::index,
    element_index
  );
}

template<typename Entry>
static inline
WriteContext<Entry> write_message_start(
  void *writer_ptr,
  WriterFn *writer_fn
) noexcept {
  using SchemaDescription = typename svf::GetSchemaFromType<Entry>::SchemaDescription;
  WriteContext<Entry> ctx_value = {};
  SVFRT_write_message_implementation(
    &ctx_value,
    writer_ptr,
    writer_fn,
    { SchemaDescription::schema_binary_array, SchemaDescription::schema_binary_size },
    SchemaDescription::template PerType<Entry>::name_hash
  );
  return ctx_value;
}

template<typename T, typename E>
static inline
Reference<T> write_reference(
  WriteContext<E> *ctx,
  T const *pointer
) noexcept {
  auto result = SVFRT_write_reference(ctx, (void *) pointer, sizeof(T));
  return { result.data_offset_complement };
}

template<typename T>
static inline
void write_message_end(
  WriteContext<T> *ctx,
  T const *pointer
) noexcept {
  SVFRT_write_message_end(ctx, (void *) pointer, sizeof(T));
}

template<typename T, typename E>
static inline
Sequence<T> write_sequence(
  WriteContext<E> *ctx,
  T const *pointer,
  U32 count
) noexcept {
  auto result = SVFRT_write_sequence(ctx, (void *) pointer, sizeof(T), count);
  return {
    /*.data_offset_complement =*/ result.data_offset_complement,
    /*.count =*/ result.count,
  };
}

template<typename T, typename E, int N>
static inline
Sequence<T> write_fixed_size_array(
  WriteContext<E> *ctx,
  T const (&array)[N]
) noexcept {
  return write_sequence(ctx, (T const *) array, (U32) N);
}

// Same as `write_fixed_size_array`, but with casting. Intended for use with
// C++ string literals.
template<typename S, typename T, typename E, int N>
static inline
Sequence<S> write_fixed_size_string(
  WriteContext<E> *ctx,
  T const (&array)[N]
) noexcept {
  static_assert(sizeof(typename IsPrimitive<S>::Yes) > 0);
  static_assert(sizeof(S) == sizeof(T));

  return write_sequence(ctx, (S const *) array, (U32) N);
}

template<typename T, typename E>
static inline
void write_sequence_element(
  WriteContext<E> *ctx,
  T const *pointer,
  Sequence<T> *inout_sequence
) noexcept {
  // TODO: this back-and-forth conversion sure is clumsy. Improve this somehow?
  SVFRT_Sequence sequence = {
    /*.data_offset_complement =*/ inout_sequence->data_offset_complement,
    /*.count =*/ inout_sequence->count,
  };

  SVFRT_write_sequence_element(ctx, (void *) pointer, sizeof(T), &sequence);

  inout_sequence->data_offset_complement = sequence.data_offset_complement;
  inout_sequence->count = sequence.count;
}

} // namespace runtime
} // namespace svf

#endif // __cplusplus

#endif // SVF_RUNTIME_HPP
