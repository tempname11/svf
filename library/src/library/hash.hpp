#pragma once
#include <cstring>
#include "../platform.hpp"
#include "range.hpp"

// Note: 64 bits is not enough for all use cases.

namespace hash64 {

U64 const fnv_offset_basis = 14695981039346656037ull;
U64 const fnv_prime = 1099511628211ull;

static inline
U64 begin() {
  return fnv_offset_basis;
}

static inline
void add_bytes(U64 *h, Bytes range) {
  for (U64 i = 0; i < range.count; i++) {
    *h = *h ^ range.pointer[i];
    *h = *h * fnv_prime;
  }
}

static inline
U64 from_name(Range<U8> name) {
  auto hash = hash64::begin();
  hash64::add_bytes(&hash, name);
  return hash;
}

static inline
U64 from_cstr(char const *cstr) {
  return from_name({(Byte *) cstr, strlen(cstr)});
}

}
