#include <cstdio>
#include <src/svf_runtime.hpp>
#include <src/library.hpp>

void test_write(svf::runtime::WriterFn *writer_fn, void *writer_ptr);
void test_read(vm::LinearArena *arena, Bytes input_range);

int main(int /*argc*/, char */*argv*/[]) {
  auto arena_value = vm::create_linear_arena(1ull << 20);
  auto arena = &arena_value;
  if (!arena->reserved_range.pointer) {
    printf("Failed to create arena.\n");
    return 1;
  }

  auto base = vm::realign(arena, svf::runtime::MESSAGE_PART_ALIGNMENT);
  auto writer_fn = [](void *writer_ptr, SVFRT_Bytes src) {
    auto arena = (vm::LinearArena *) writer_ptr;
    auto dst = vm::many<U8>(arena, src.count);
    range_copy(dst, {src.pointer, src.count});
    return safe_int_cast<U32>(src.count);
  };

  test_write(writer_fn, (void *) arena);

  auto end = arena->reserved_range.pointer + arena->waterline;
  auto write_result = Bytes {
    .pointer = (Byte *) base,
    .count = offset_between<U32>(base, end)
  };

  test_read(arena, write_result);

  printf("Test passed.\n");

  return 0;
}
