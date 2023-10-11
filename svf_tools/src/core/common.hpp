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
U32 get_compatibility_work_base(Bytes schema_bytes, svf::META::Schema *in_schema) {
  // See #compatibility-work. For the "base" we will take the scenario where
  // the schema is checked for compatibility with one equivalent to itself.
  //
  uint32_t result = in_schema->structs.count; // [1]

  auto the_structs = to_range(schema_bytes, in_schema->structs);
  for (UInt i = 0; i < the_structs.count; i++) {
    auto definition = the_structs.pointer + i;
    result += definition->fields.count * definition->fields.count; // [2]
  }

  ASSERT((uint64_t) result * SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR < UINT32_MAX);

  return result;
}

static inline
UInt get_min_read_scratch_memory_size(svf::META::Schema *in_schema) {
  // #scratch-memory-partitions.
  return (
    (sizeof(U32) - 1) // In case of misalignment.
    + in_schema->structs.count * sizeof(U32) * 4 // Strides, matches, 2x queue.
    + in_schema->choices.count * sizeof(U32) * 3 // Matches, 2x queue.
  );
}
