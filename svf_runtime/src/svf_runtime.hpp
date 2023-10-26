#ifndef SVF_RUNTIME_HPP
#define SVF_RUNTIME_HPP

// Note: this file is intended to be compiled as C++11 or later.
//
// TODO @support: check if all popular compilers can actually build it.

#ifdef __cplusplus

#include <cstdint>

#ifndef SVFRT_SINGLE_FILE
  #include "svf_runtime.h"
#endif

namespace svf {
namespace runtime {

#ifndef SVF_COMMON_CPP_TYPES_INCLUDED
#define SVF_COMMON_CPP_TYPES_INCLUDED
#pragma pack(push, 1)

template<typename T>
struct Reference {
  uint32_t data_offset_complement;
};

template<typename T>
struct Sequence {
  uint32_t data_offset_complement;
  uint32_t count;
};

template<typename T> struct GetSchemaFromType;

#pragma pack(pop)
#endif // SVF_COMMON_CPP_TYPES_INCLUDED

template<typename T> struct IsPrimitive { using No = char; };
template<> struct IsPrimitive<uint8_t> { using Yes = char; };
template<> struct IsPrimitive<uint16_t> { using Yes = char; };
template<> struct IsPrimitive<uint32_t> { using Yes = char; };
template<> struct IsPrimitive<uint64_t> { using Yes = char; };
template<> struct IsPrimitive<int8_t> { using Yes = char; };
template<> struct IsPrimitive<int16_t> { using Yes = char; };
template<> struct IsPrimitive<int32_t> { using Yes = char; };
template<> struct IsPrimitive<int64_t> { using Yes = char; };
template<> struct IsPrimitive<float> { using Yes = char; };
template<> struct IsPrimitive<double> { using Yes = char; };

constexpr size_t MESSAGE_PART_ALIGNMENT = SVFRT_MESSAGE_PART_ALIGNMENT;
typedef SVFRT_MessageHeader MessageHeader;
static_assert(sizeof(MessageHeader) % MESSAGE_PART_ALIGNMENT == 0);

template<typename T> struct Range {
  T *pointer;
  uint32_t count;
};

typedef Range<uint8_t> Bytes;
typedef SVFRT_ReadContext ReadContext;
typedef SVFRT_AllocatorFn AllocatorFn;
typedef SVFRT_WriterFn WriterFn;
typedef SVFRT_SchemaLookupFn SchemaLookupFn;

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
  SVFRT_ErrorCode error_code; // TODO: an enum class would be nice here.
  T const *entry;
  void *allocation;
  CompatibilityLevel compatibility_level;
  ReadContext context;
};

template<typename T> struct WriteContext: SVFRT_WriteContext {};

template<typename Entry>
static inline
ReadMessageResult<Entry> read_message(
  Range<uint8_t> message,
  Range<uint8_t> scratch,
  CompatibilityLevel required_level,
  AllocatorFn *allocator_fn = NULL,
  void *allocator_ptr = NULL,
  SchemaLookupFn *schema_lookup_fn = NULL,
  void *schema_lookup_ptr = NULL
) noexcept {
  using SchemaDescription = typename svf::runtime::GetSchemaFromType<Entry>::SchemaDescription;
  SVFRT_ReadMessageParams params;
  SVFRT_ReadMessageResult result;
  params.expected_schema_content_hash = SchemaDescription::content_hash;
  params.expected_schema_struct_strides.pointer = (uint32_t *) SchemaDescription::schema_struct_strides;
  params.expected_schema_struct_strides.count = SchemaDescription::schema_struct_count;
  params.expected_schema.pointer = (uint8_t *) SchemaDescription::schema_binary_array;
  params.expected_schema.count = SchemaDescription::schema_binary_size;
  params.required_level = (SVFRT_CompatibilityLevel) required_level;
  params.entry_struct_id = SchemaDescription::template PerType<Entry>::type_id,
  params.entry_struct_index = SchemaDescription::template PerType<Entry>::index,
  params.max_schema_work = SchemaDescription::compatibility_work_base * SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR;
  params.max_recursion_depth = SVFRT_DEFAULT_MAX_RECURSION_DEPTH;
  params.max_output_size = SVFRT_NO_SIZE_LIMIT;
  params.allocator_fn = allocator_fn;
  params.allocator_ptr = allocator_ptr;
  params.schema_lookup_fn = schema_lookup_fn;
  params.schema_lookup_ptr = schema_lookup_ptr;
  SVFRT_read_message(
    &params,
    &result,
    SVFRT_Bytes {
      /*.pointer =*/ message.pointer,
      /*.count =*/ message.count,
    },
    SVFRT_Bytes {
      /*.pointer =*/ scratch.pointer,
      /*.count =*/ scratch.count,
    }
  );
  return ReadMessageResult<Entry> {
    /*.error_code =*/ result.error_code,
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
  uint32_t element_index
) noexcept {
  using SchemaDescription = typename svf::runtime::GetSchemaFromType<T>::SchemaDescription;
  return (T const *) SVFRT_read_sequence_element(
    ctx,
    SVFRT_Sequence { sequence.data_offset_complement, sequence.count },
    SchemaDescription::template PerType<T>::index,
    element_index
  );
}

template<typename Entry>
static inline
WriteContext<Entry> write_start(
  WriterFn *writer_fn,
  void *writer_ptr
) noexcept {
  using SchemaDescription = typename svf::runtime::GetSchemaFromType<Entry>::SchemaDescription;
  WriteContext<Entry> ctx_value = {};
  SVFRT_write_start(
    &ctx_value,
    writer_fn,
    writer_ptr,
    SchemaDescription::content_hash,
    { SchemaDescription::schema_binary_array, SchemaDescription::schema_binary_size },
    SchemaDescription::template PerType<Entry>::type_id
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
void write_finish(
  WriteContext<T> *ctx,
  T const *pointer
) noexcept {
  SVFRT_write_finish(ctx, (void *) pointer, sizeof(T));
}

template<typename T, typename E>
static inline
Sequence<T> write_sequence(
  WriteContext<E> *ctx,
  T const *pointer,
  uint32_t count
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
  return write_sequence(ctx, (T const *) array, (uint32_t) N);
}

// Same as `write_fixed_size_array`, but with casting. Intended for use with
// C++ string literals.
template<typename S, typename T, typename E, int N>
static inline
Sequence<S> write_fixed_size_string(
  WriteContext<E> *ctx,
  T const (&array)[N],
  bool termination_type = !NULL // Intended use: NULL for null-terminated strings, !NULL otherwise.
) noexcept {
  static_assert(sizeof(typename IsPrimitive<S>::Yes) > 0);
  static_assert(sizeof(S) == sizeof(T));
  static_assert(N > 0);

  return write_sequence(ctx, (S const *) array, (uint32_t) (termination_type == NULL ? N - 1 : N));
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
