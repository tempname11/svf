#include <cstdio>
#include <windows.h>
#include <src/library.hpp>

void abort_this_process(char const *message, char const *filename, UInt line) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "ASSERT(%s)\n\nat %s:%zu", message, filename, line);
  MessageBoxA(0, buffer, "Assertion failed", MB_OK | MB_ICONERROR);
  _set_abort_behavior( 0, _WRITE_ABORT_MSG);
  abort();
}

namespace vm {

LinearArena create_linear_arena(UInt reserved_size_in_bytes) {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  UInt page_size = (UInt) info.dwPageSize;

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
    .reserved_range = Bytes {
      .pointer = (Byte *)allocation,
      .count = align_up(reserved_size_in_bytes, page_size),
    },
    .page_size = page_size,
    .committed_page_boundary = 0,
    .waterline = 0,
  };
}

void destroy_linear_arena(LinearArena *arena) {
  auto result = VirtualFree(
    (void *) arena->reserved_range.pointer,
    0,
    MEM_RELEASE
  );
  ASSERT(result);
}

void *allocate_manually(
  LinearArena *arena,
  UInt size_in_bytes,
  UInt alignment_in_bytes
) {
  ASSERT(alignment_in_bytes > 0);
  ASSERT(alignment_in_bytes <= alignof(max_align_t));

  UInt allocation_offset = align_up(arena->waterline, alignment_in_bytes);
  UInt new_waterline = allocation_offset + size_in_bytes; // May overflow.

  if (0
    || new_waterline < allocation_offset // Check for overflow.
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
