#pragma once
#include <src/library.hpp>
#include <src/svf_meta.hpp>
#include <src/svf_runtime.h>
#include "../core.hpp"

namespace core {

template<typename T>
inline static
Range<T> to_range(Bytes bytes, svf::runtime::Sequence<T> sequence) {
  return range_from_bytes<T>(
    range_subrange(bytes, ~sequence.data_offset_complement, sequence.count * sizeof(T))
  );
}

enum class TypePlurality {
  zero,
  one,
  tag_and_payload,
};

struct TypePluralityAndSize {
  TypePlurality plurality;
  UInt size;
};

TypePluralityAndSize get_plurality(
  Range<svf::Meta::StructDefinition> structs,
  Range<svf::Meta::ChoiceDefinition> choices,
  svf::Meta::Type_tag in_tag,
  svf::Meta::Type_payload *in_payload
);

static inline
U64 get_content_hash(Bytes schema_bytes) {
  auto result = hash64::begin();
  hash64::add_bytes(&result, schema_bytes);
  ASSERT(result != 0);
  return result;
}

// Internal usage only. Both schemas are assumed to be valid.
static inline
U32 get_compatibility_work(Bytes schema_src, Bytes schema_dst) {
  // Note: returns an upper bound, the real number will depend on how many
  // structs are reachable from the entry.

  // TODO @proper-alignment.
  auto definition_src = (svf::Meta::SchemaDefinition *) (
    schema_src.pointer +
    schema_src.count -
    sizeof(svf::Meta::SchemaDefinition)
  );

  // TODO @proper-alignment.
  auto definition_dst = (svf::Meta::SchemaDefinition *) (
    schema_dst.pointer +
    schema_dst.count -
    sizeof(svf::Meta::SchemaDefinition)
  );

  // See #compatibility-work.
  uint32_t result = definition_src->structs.count; // [1]

  auto structs_src = to_range(schema_src, definition_src->structs);
  auto structs_dst = to_range(schema_dst, definition_dst->structs);

  for (UInt i = 0; i < structs_dst.count; i++) {
    auto definition_dst = structs_dst.pointer + i;

    bool found = false;
    for (UInt j = 0; j < structs_src.count; j++) {
      auto definition_src = structs_src.pointer + j;

      if (definition_dst->typeId != definition_src->typeId) {
        continue;
      }

      ASSERT(!found);
      found = true;
      result += definition_src->fields.count * definition_dst->fields.count; // [2]
      break;
    }

    ASSERT(found);
  }

  // TODO: this probably needs to be more strict.
  ASSERT((uint64_t) result * SVFRT_DEFAULT_COMPATIBILITY_TRUST_FACTOR < UINT32_MAX);

  return result;
}

// Internal usage only. Schema assumed to be valid.
static inline
U32 get_compatibility_work_base(Bytes schema_bytes) {
  // For the "base" we will take the scenario where
  // the schema is checked for compatibility with one equivalent to itself.
  return get_compatibility_work(schema_bytes, schema_bytes);
}

static inline
UInt get_min_read_scratch_memory_size(svf::Meta::SchemaDefinition *definition) {
  // #scratch-memory-partitions.
  return (
    (sizeof(U32) - 1) // In case of misalignment.
    + definition->structs.count * sizeof(U32) * 4 // Strides, matches, 2x queue.
    + definition->choices.count * sizeof(U32) * 3 // Matches, 2x queue.
  );
}

} // namespace core
