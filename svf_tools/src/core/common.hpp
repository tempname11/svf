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

  // TODO @proper-alignment: struct access.
  auto definition_src = (svf::Meta::SchemaDefinition *) (
    schema_src.pointer +
    schema_src.count -
    sizeof(svf::Meta::SchemaDefinition)
  );

  // TODO @proper-alignment: struct access.
  auto definition_dst = (svf::Meta::SchemaDefinition *) (
    schema_dst.pointer +
    schema_dst.count -
    sizeof(svf::Meta::SchemaDefinition)
  );

  uint32_t result = definition_src->structs.count; // See #compatibility-work-entry.

  auto structs_src = to_range(schema_src, definition_src->structs);
  auto structs_dst = to_range(schema_dst, definition_dst->structs);
  auto choices_src = to_range(schema_src, definition_src->choices);
  auto choices_dst = to_range(schema_dst, definition_dst->choices);

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

      // See #compatibility-work-fields.
      result += 2 * definition_src->fields.count * definition_dst->fields.count;

      break;
    }

    ASSERT(found);
  }

  for (UInt i = 0; i < choices_dst.count; i++) {
    auto definition_dst = choices_dst.pointer + i;

    bool found = false;
    for (UInt j = 0; j < choices_src.count; j++) {
      auto definition_src = choices_src.pointer + j;

      if (definition_dst->typeId != definition_src->typeId) {
        continue;
      }

      ASSERT(!found);
      found = true;

      // See #compatibility-work-options.
      result += 2 * definition_src->options.count * definition_dst->options.count;

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
UInt get_min_read_scratch_memory_size(Bytes schema, svf::Meta::SchemaDefinition *definition) {
  U32 total_fields = 0;
  auto structs = to_range(schema, definition->structs);
  for (U32 i = 0; i < structs.count; i++) {
    auto definition = structs.pointer + i;
    total_fields += definition->fields.count;
  }

  U32 total_options = 0;
  auto choices = to_range(schema, definition->choices);
  for (U32 i = 0; i < choices.count; i++) {
    auto definition = choices.pointer + i;
    total_options += definition->options.count;
  }

  // #scratch-memory-partitions.
  return (
    (sizeof(U32) - 1) // In case of misalignment.
    + definition->structs.count * sizeof(U32) * 5 // Strides, matches, queue (x2), field match header.
    + definition->choices.count * sizeof(U32) * 4 // Matches, queue (x2), options match header.
    + total_fields * sizeof(U32)
    + total_options * sizeof(U32)
    + total_options * sizeof(U8)
  );
}

} // namespace core
