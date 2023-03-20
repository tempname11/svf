#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <src/library.hpp>

void abort_this_process(char const *message, char const *filename, UInt line) {
  fprintf(stdout, "ASSERT(%s)\n\nat %s:%zu\n", message, filename, line);
  fflush(stdout);
  abort();
}

namespace vm {

LinearArena create_linear_arena(UInt reserved_size_in_bytes) {
  UInt page_size = (UInt) sysconf(_SC_PAGE_SIZE);

  void *allocation = mmap(0, reserved_size_in_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

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
  auto result = munmap(arena->reserved_range.pointer, arena->reserved_range.count);
  ASSERT(result == 0);
}

void *allocate_manually(
  LinearArena *arena,
  UInt size_in_bytes,
  UInt alignment_in_bytes
) {
  // Note: this implementation is vrtually the same as for Windows, except that
  // `VirtualAlloc` that does commit is replaced with `mprotect` here.

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
    auto result = mprotect(
      arena->reserved_range.pointer + arena->committed_page_boundary,
      new_page_boundary - arena->committed_page_boundary,
      PROT_READ | PROT_WRITE
    );
    if (result != 0) {
      return 0;
    }
  }

  arena->waterline = new_waterline;
  arena->committed_page_boundary = new_page_boundary;

  return (void *)(arena->reserved_range.pointer + allocation_offset);
}

} // namespace vm
