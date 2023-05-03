// AUTOGENERATED by svfc.
#pragma once
#include <cstdint>

namespace svf {

#ifndef SVF_AUTOGENERATED_COMMON_TYPES
#define SVF_AUTOGENERATED_COMMON_TYPES
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
struct Array {
  uint32_t pointer_offset;
  uint32_t count;
};
#endif // SVF_AUTOGENERATED_COMMON_TYPES

namespace schema_a0 {
#pragma pack(push, 1)

// Forward declarations
struct A;

// Hashes of top level definition names
U64 const A_name_hash = 0xAF63FC4C860222ECull;

// Full declarations
struct A {
  U64 left;
  U64 right;
};

#pragma pack(pop)
}

}
