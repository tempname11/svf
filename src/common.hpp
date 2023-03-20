#pragma once

#include <cstdio>
#include <cstdint>

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

static inline
size_t align_up(size_t value, size_t alignment) {
	ASSERT(value + alignment > value); // sanity check
	return value + alignment - 1 - (value - 1) % alignment;
}
// Untested, but should work.

namespace VM {

struct LinearArena {
	Range<Byte> reserved_range;
	size_t page_size;
	size_t committed_page_boundary;
	size_t waterline;
};
// `reserved_range.pointer` must be be 16-byte-aligned (likely OS page-aligned).
// Arena is considered invalid, if `reserved_range.pointer` is 0.

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
T *allocate_many(LinearArena *arena, size_t count) {
	size_t total_size = count * sizeof(T);
	if (total_size / sizeof(T) != count) {
		return 0;
	}
	return (T *)allocate_manually(arena, total_size, alignof(T));
}

} // namespace VM
