#pragma once
#include <src/library.hpp>
#include <src/svf_meta.hpp>
#include "../core.hpp"

template<typename T>
inline static
Range<T> to_range(Bytes bytes, svf::Array<T> array) {
  return range_from_bytes<T>(
    range_subrange(bytes, array.data_offset, array.count * sizeof(T))
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
UInt get_min_read_scratch_memory_size(svf::META::Schema *in_schema) {
  return (
    in_schema->structs.count * sizeof(U32) * 2 // index and stride
    + in_schema->choices.count * sizeof(U32) // index
  );
}
