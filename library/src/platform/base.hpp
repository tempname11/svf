#pragma once
#include <cstdint>
#include <cstddef>
#include <climits>

// The platforms we are going to run on are constrained.
static_assert(CHAR_BIT == 8);
static_assert(__LITTLE_ENDIAN__); // Clang-specific.
static_assert(sizeof(size_t) == sizeof(void *));
static_assert(sizeof(void *) >= 4);

// Aliases for types we will use a lot, short and consistent.
// Like other types, these have uppercase first letter naming.
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

// Not equal to `bool`, because that type might not be 1 byte.
using Bool = uint8_t;

// `Byte` is semantic sugar, used when we just refer to memory,
// regardless of the contents.
using Byte = uint8_t;

// Native integer types, used in contexts where we don't care about the exact size.
using Int = int64_t;
using UInt = uint64_t;

// A range is analogous to C++'s `std::span`. But we want to use C++'s
// standard library as little as possible, so we define our own, as a struct.
//
// The range is considered invalid, if `pointer` is 0.
template<typename T>
struct Range {
  T* pointer;
  UInt count;
};

namespace internal_newtype_test {
  // Test whether newtypes work as we expect.

  struct NewtypeU8 { U8 value; };
  struct NewtypeU16 { U16 value; };
  struct NewtypeU32 { U32 value; };
  struct NewtypeU64 { U64 value; };

  static_assert(sizeof(NewtypeU8) == sizeof(U8));
  static_assert(alignof(NewtypeU8) == alignof(U8));

  static_assert(sizeof(NewtypeU16) == sizeof(U16));
  static_assert(alignof(NewtypeU16) == alignof(U16));

  static_assert(sizeof(NewtypeU32) == sizeof(U32));
  static_assert(alignof(NewtypeU32) == alignof(U32));

  static_assert(sizeof(NewtypeU64) == sizeof(U64));
  static_assert(alignof(NewtypeU64) == alignof(U64));
}
