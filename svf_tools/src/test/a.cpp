#include <cstdio>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <generated/hpp/a0.hpp>
#include <generated/hpp/a1.hpp>
#include <src/svf_runtime.hpp>
#include <src/library.hpp>

void test_write(svf::runtime::WriterFn *writer_fn, void *writer_ptr) {
  namespace schema = svf::A0;

  auto ctx = svf::runtime::write_message_start<schema::Entry>(
    writer_ptr,
    writer_fn
  );

  schema::Target target0 = {
    .value = 0x1111111111111111ull,
  };

  schema::Entry entry = {
    .pointer = svf::runtime::write_pointer(&ctx, &target0),
    .some_struct = {
      .array = {},
      .some_choice_enum = schema::SomeChoice_enum::target,
      .some_choice_union = {
        .target = {
          .value = 0x4444444444444444ull,
        },
      },
    },
  };

  for (UInt i = 0; i < 2; i++) {
    schema::Target target = {
      .value = (i + 2) * 0x1111111111111111ull,
    };
    svf::runtime::write_array_element(&ctx, &target, &entry.some_struct.array);
  }

  svf::runtime::write_message_end(&ctx, &entry);

  if (!ctx.finished) {
    printf("Write was not finished.\n");
    abort_this_process();
  }
}

void test_read(vm::LinearArena *arena, Bytes input_range) {
  namespace schema = svf::A1;

  vm::realign(arena);
  auto scratch_memory = vm::many<U8>(
    arena,
    schema::SchemaDescription::min_read_scratch_memory_size
  );
  auto read_result = svf::runtime::read_message<schema::Entry>(
    svf::runtime::Range<U8> { input_range.pointer, input_range.count },
    svf::runtime::Range<U8> { scratch_memory.pointer, scratch_memory.count },
    svf::runtime::CompatibilityLevel::compatibility_binary
  );

  if (!read_result.entry) {
    printf("Read result is invalid.\n");
    abort_this_process();
  }

  auto ctx = &read_result.context;
  auto entry = read_result.entry;

  auto target0 = svf::runtime::read_pointer(ctx, entry->pointer);
  ASSERT(target0);
  ASSERT(target0->value == 0x1111111111111111ull);

  ASSERT(entry->some_struct.array.count == 2);
  auto e0 = svf::runtime::read_array(ctx, entry->some_struct.array, 0);
  auto e1 = svf::runtime::read_array(ctx, entry->some_struct.array, 1);
  ASSERT(e0 && e1);
  ASSERT(e0->value == 0x2222222222222222ull);
  ASSERT(e1->value == 0x3333333333333333ull);

  ASSERT(entry->some_struct.some_choice_enum == schema::SomeChoice_enum::target);
  ASSERT(entry->some_struct.some_choice_union.target.value == 0x4444444444444444ull);
}