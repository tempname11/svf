#include <cstdio>
#include <windows.h>
#include <src/library.hpp>

// void report_error_details_gui(char const *message) {
//   MessageBoxA(0, message, "Internal error. Please report this.", MB_OK | MB_ICONERROR);
// }

ErrorReportFn _internal_report_error_details = report_error_details_default;

Impossible terminate() {
  // MS quote:
  //
  // For Windows compatibility reasons, when abort calls _exit, it may invoke
  // the Windows ExitProcess API, which in turn allows DLL termination routines
  // to run. Destructors aren't run in the executable, but the same may not be
  // true of DLLs loaded in the executable's process space. This behavior
  // doesn't strictly conform to the C++ standard. To immediately terminate a
  // process including any DLLs, use the Windows TerminateProcess API.

  TerminateProcess(GetCurrentProcess(), 3);
  // 3: a weird Windows convention.
  // "The C runtime abort function terminates the process with exit code 3."
  // See https://devblogs.microsoft.com/oldnewthing/20110519-00/

  return Impossible();
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
