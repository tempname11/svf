#pragma once
#include "../platform.hpp"

// `Range` itself is defined in `platform`, because of dependencies there.
typedef Range<Byte> Bytes;

template<typename T>
static inline
Range<T> range_subrange(Range<T> range, UInt offset, UInt count) {
  ASSERT(offset <= range.count);
  ASSERT(offset + count <= range.count);
  return {
    .pointer = range.pointer + offset,
    .count = count,
  };
}

template<typename T>
static inline
void range_copy(Range<T> dst, Range<T> src) {
  ASSERT(dst.count >= src.count);
  if (src.count > 0) {
    memcpy(dst.pointer, src.pointer, src.count);
  }
}

template<typename T>
static inline
bool range_equal(Range<T> dst, Range<T> src) {
  return (
    (src.count == dst.count) &&
    (memcmp(src.pointer, dst.pointer, src.count) == 0)
  );
}

template<typename T>
static inline
Bytes range_to_bytes(Range<T> range) {
  return {
    .pointer = (Byte *) range.pointer,
    .count = range.count * sizeof(T),
  };
}

template<typename T>
static inline
Range<T> range_from_bytes(Bytes bytes) {
  ASSERT(bytes.count % sizeof(T) == 0);
  return {
    .pointer = (T *) bytes.pointer,
    .count = bytes.count / sizeof(T),
  };
}

static inline
Range<U8> range_from_cstr(char const *cstr) {
  return {
    .pointer = (U8 *) cstr,
    .count = strlen(cstr),
  };
}
