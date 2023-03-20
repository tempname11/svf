#pragma once
#include <cstddef>
#include "base.hpp"

namespace vm {

// This is a trick that vastly simplifies memory management. It combines the
// idea of an arena with the VM implementation that is very practical.
//
// The "linear" arena always allocates memory in a linear way,
// i.e. contiguously from the beginning of the reserved range.
//
// Using virtual memory, it's possible to reserve a large range of memory, and
// then commit it in smaller chunks. This allows for the arena to grow
// dynamically, but still allocate memory in a linear way.
//
// The arena is considered invalid, if `reserved_range` is invalid.
// `reserved_range.pointer` must be `max_align_t`-aligned (likely OS page-aligned).
struct LinearArena {
  Range<Byte> reserved_range;
  UInt page_size;
  UInt committed_page_boundary;
  UInt waterline;
};

// Creates a linear arena with the given reserved size in bytes.
// Typically, the size would as large as the program could ever need.
//
// May return an invalid arena.
LinearArena create_linear_arena(UInt reserved_size_in_bytes);

void destroy_linear_arena(LinearArena *arena);

// Allocate bytes manually.
//
// May fail, returning 0.
void *allocate_manually(
  LinearArena *arena,
  UInt size_in_bytes,
  UInt alignment_in_bytes
);

// Allocate a single object.
//
// May fail, returning 0.
template<typename T>
static inline
T *allocate_one(LinearArena *arena) {
  return (T *)allocate_manually(arena, sizeof(T), alignof(T));
}

// Allocate a contiguous array of objects.
//
// May fail, returning an empty range.
template<typename T>
static inline
Range<T> allocate_many(LinearArena *arena, UInt count) {
  UInt total_size = count * sizeof(T);
  if (total_size / sizeof(T) != count) {
    return {};
  }
  auto pointer = (T *)allocate_manually(arena, total_size, alignof(T));
  return Range<T> {
    .pointer = pointer,
    .count = count,
  };
}

// [[Deprecated.]]
//
// This is a weird one, but may be useful.
// Anticipate some next allocation, and return a pointer to it in advance.
// This is only possible by forcing the arena to realign to `max_align_t`.
static inline
void *next_allocation(LinearArena *arena) {
  auto pointer = allocate_manually(arena, 0, alignof(max_align_t));
  ASSERT(pointer);
  return pointer;
}

static inline
void *realign(LinearArena *arena, UInt alignment = alignof(max_align_t)) {
  auto pointer = allocate_manually(arena, 0, alignment);
  ASSERT(pointer);
  return pointer;
}

// Allocate a single object. "Never" fails by asserting the result.
// Use this for smaller allocations, where checking the result is not worth it.
template<typename T>
static inline
T *one(LinearArena *arena) {
  auto result = allocate_one<T>(arena);
  ASSERT(result);
  return result;
}

// Allocate a contiguous array of objects. "Never" fails by asserting the result.
// Use this for smaller allocations, where checking the result is not worth it.
template<typename T>
static inline
Range<T> many(LinearArena *arena, UInt count) {
  auto result = allocate_many<T>(arena, count);
  ASSERT(result.pointer);
  return result;
}

} // namespace vm
