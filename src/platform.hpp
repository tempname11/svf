#pragma once
#include <cstdio>
#include <cstdint>

using Bool = uint8_t;
using Byte = uint8_t;
using Int = int64_t;

using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

using I8 = int8_t;
using I16 = int16_t;
using I32 = int32_t;
using I64 = int64_t;

void abort_this_process(char const *cstr_source_code);

#ifdef COMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT
#define ASSERT(condition) {if(!(condition)) abort_this_process(#condition);}
#else
#define ASSERT(...)
#endif

template<typename T>
struct Range {
  T* pointer;
  size_t count;
};
// Range is considered invalid, if `pointer` is 0.

template<typename T>
struct DynamicArray {
  T* pointer;
  size_t count;
  size_t capacity;
};
// This is a temporary solution; it is too coupled to VM stuff.
// Does the job for now.
// Is considered invalid, if `pointer` is 0.

static inline
size_t align_up(size_t value, size_t alignment) {
  ASSERT(value + alignment > value); // sanity check
  return value + alignment - 1 - (value - 1) % alignment;
}
// Untested, but should work.

template<typename T1, typename T2>
static inline
T1 safe_int_cast(T2 value) {
  auto forward = T1(value);
  auto back = T2(forward);
  ASSERT(back == value);
  ASSERT((value >= 0 && back >= 0) || (value < 0 && back < 0));
  return T1(value);
}

namespace vm {

struct LinearArena {
  Range<Byte> reserved_range;
  size_t page_size;
  size_t committed_page_boundary;
  size_t waterline;
};
// `reserved_range.pointer` must be be 16-byte-aligned (likely OS page-aligned).
// Arena is considered invalid, if `reserved_range` is invalid.

LinearArena create_linear_arena(size_t reserved_size_in_bytes);
// May return invalid arena.

bool change_allocation_size(LinearArena *arena, size_t size_in_bytes);
// May fail, returning `false`.

void *allocate_manually(
  LinearArena *arena,
  size_t size_in_bytes,
  size_t alignment_in_bytes
);
// May fail, returning 0.

template<typename T>
static inline
T *allocate_one(LinearArena *arena) {
  return (T *)allocate_manually(arena, sizeof(T), alignof(T));
}

template<typename T>
static inline
Range<T> allocate_many(LinearArena *arena, size_t count) {
  size_t total_size = count * sizeof(T);
  if (total_size / sizeof(T) != count) {
    return {};
  }
  auto pointer = (T *)allocate_manually(arena, total_size, alignof(T));
  return Range<T> {
    .pointer = pointer,
    .count = count,
  };
}

} // namespace vm

static inline
size_t round_up_to_power_of_two(size_t size) {
  ASSERT(size < SIZE_MAX / 2); // sanity check
  size_t result = 1;
  while (result < size) {
    result *= 2;
  }
  return result;
}

template<typename T>
static inline
void dynamic_resize(
  DynamicArray<T> *dynamic,
  vm::LinearArena *arena,
  size_t required_count
)
// May fail, leaving `dynamic` in invalid state.
//
// Note: old values are left intact. This may be surprising.
{
  if (dynamic->capacity >= required_count) {
    return;
  }
  size_t final_count = round_up_to_power_of_two(required_count);

  auto reserved_range = allocate_many<T>(arena, final_count);
  if (!reserved_range.pointer) {
    *dynamic = {};
  }

  memcpy(reserved_range.pointer, dynamic->pointer, dynamic->count * sizeof(T));
  *dynamic = {
    .pointer = reserved_range.pointer,
    .count = dynamic->count,
    .capacity = reserved_range.count,
  };
}

namespace hash64 {

U64 const fnv_offset_basis = 14695981039346656037ull;
U64 const fnv_prime = 1099511628211ull;

static inline
U64 begin() {
  return fnv_offset_basis;
}

static inline
void add_bytes(U64 *h, Range<Byte> range) {
  for (U64 i = 0; i < range.count; i++) {
    *h = *h ^ range.pointer[i];
    *h = *h * fnv_prime;
  }
}

}
