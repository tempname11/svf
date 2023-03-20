#pragma once
#include <cstring>
#include "../platform.hpp"

// Crude dynamic array.
//
// This is meant to be used with an arena where you do not care about the
// memory consumption that much.
template<typename T>
struct CrudeDynamicArray {
  T* pointer;
  UInt count;
  UInt capacity;
};

// Makes sure that the array has enough capacity to hold the required count.
// Preserves the contents of the old array even on reallocation.
template<typename T>
static inline
void ensure_size(
  CrudeDynamicArray<T> *dynamic,
  vm::LinearArena *arena,
  UInt required_count
) {
  // If capacity is already enough, do nothing.
  if (dynamic->capacity >= required_count) {
    return;
  }

  UInt final_count = round_up_to_power_of_two(required_count);
  auto reserved_range = many<T>(arena, final_count);
  if (dynamic->count != 0) {
    memcpy(reserved_range.pointer, dynamic->pointer, dynamic->count * sizeof(T));
  }

  *dynamic = {
    .pointer = reserved_range.pointer,
    .count = dynamic->count,
    .capacity = reserved_range.count,
  };
}

template<typename T>
static inline
void ensure_one_more(
  CrudeDynamicArray<T> *dynamic,
  vm::LinearArena *arena
) {
  ensure_size(dynamic, arena, dynamic->count + 1);
}
