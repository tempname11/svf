#include <cstring>

#include "platform.hpp"

// Temporary place to keep utilities.

static inline
U64 compute_name_hash(Range<Byte> name) {
  auto hash = hash64::begin();
  hash64::add_bytes(&hash, name);
  return hash;
}

static inline
U64 compute_cstr_hash(char const *cstr) {
	return compute_name_hash({(Byte *) cstr, strlen(cstr)});
}

static inline
U32 offset_between(void *base, void *pointer) {
	auto result = (Byte *) pointer - (Byte *) base;
	return safe_int_cast<U32>(result);
}