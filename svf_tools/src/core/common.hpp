#pragma once
#include <src/library.hpp>
#include <src/svf_meta.hpp>
#include <src/svf_runtime.h>
#include "../core.hpp"

template<typename T>
inline static
Range<T> to_range(Bytes bytes, svf::Sequence<T> sequence) {
  return range_from_bytes<T>(
    range_subrange(bytes, ~sequence.data_offset_complement, sequence.count * sizeof(T))
  );
}

enum class TypePlurality {
  zero,
  one,
  enum_and_union,
};

struct TypePluralityAndSize {
  TypePlurality plurality;
  UInt size;
};

TypePluralityAndSize get_plurality(
  Range<svf::META::StructDefinition> structs,
  Range<svf::META::ChoiceDefinition> choices,
  svf::META::Type_enum in_enum,
  svf::META::Type_union *in_union
);

static inline
U64 get_content_hash(Bytes schema_bytes) {
  auto result = hash64::begin();
  hash64::add_bytes(&result, schema_bytes);
  ASSERT(result != 0);
  return result;
}

static inline
U32 get_compatibility_work_base(Bytes schema_bytes, svf::META::SchemaDefinition *definition) {
  // See #compatibility-work. For the "base" we will take the scenario where
  // the schema is checked for compatibility with one equivalent to itself.
  //
  uint32_t result = definition->structs.count; // [1]

  auto the_structs = to_range(schema_bytes, definition->structs);
  for (UInt i = 0; i < the_structs.count; i++) {
    auto definition = the_structs.pointer + i;
    result += definition->fields.count * definition->fields.count; // [2]
  }

  ASSERT((uint64_t) result * SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR < UINT32_MAX);

  return result;
}

static inline
UInt get_min_read_scratch_memory_size(svf::META::SchemaDefinition *definition) {
  // #scratch-memory-partitions.
  return (
    (sizeof(U32) - 1) // In case of misalignment.
    + definition->structs.count * sizeof(U32) * 4 // Strides, matches, 2x queue.
    + definition->choices.count * sizeof(U32) * 3 // Matches, 2x queue.
  );
}
