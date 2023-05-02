#include <windows.h>

#include "platform.hpp"

void abort_this_process(char const *cstr_source_code) {
  // Should output some message here.
  (void) cstr_source_code;

  abort();
}

namespace vm {

LinearArena create_linear_arena(size_t reserved_size_in_bytes) {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  size_t page_size = (size_t) info.dwPageSize;

  void *allocation = VirtualAlloc(
    0,
    reserved_size_in_bytes,
    MEM_RESERVE,
    PAGE_NOACCESS
  );

  if (!allocation) {
    return LinearArena {};
  }

  return LinearArena {
    .reserved_range = Range<Byte> {
      .pointer = (Byte *)allocation,
      .count = align_up(reserved_size_in_bytes, page_size),
    },
    .page_size = page_size,
    .committed_page_boundary = 0,
    .waterline = 0,
  };
}

void *allocate_manually(
  LinearArena *arena,
  size_t size_in_bytes,
  size_t alignment_in_bytes
) {
  ASSERT(alignment_in_bytes > 0);
  ASSERT(alignment_in_bytes <= 16);

  size_t allocation_offset = align_up(arena->waterline, alignment_in_bytes);
  size_t new_waterline = allocation_offset + size_in_bytes; // may overflow

  if (0
    || new_waterline < allocation_offset // check for overflow
    || new_waterline > arena->reserved_range.count
  ) {
    return 0;
  }

  auto new_page_boundary = align_up(new_waterline, arena->page_size);
  if (new_page_boundary > arena->committed_page_boundary) {
    auto result = VirtualAlloc(
      arena->reserved_range.pointer + arena->committed_page_boundary,
      new_page_boundary - arena->committed_page_boundary,
      MEM_COMMIT,
      PAGE_READWRITE
    );
    if (result == 0) {
      return 0;
    }
  }

  arena->waterline = new_waterline;
  arena->committed_page_boundary = new_page_boundary;

  return (void *)(arena->reserved_range.pointer + allocation_offset);
}

} // namespace vm