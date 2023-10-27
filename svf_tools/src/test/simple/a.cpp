#include <cstdio>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <generated/hpp/A0.hpp>
#include <generated/hpp/A1.hpp>
#include <src/svf_runtime.hpp>
#include <src/library.hpp>

void test_write(svf::runtime::WriterFn *writer_fn, void *writer_ptr) {
  namespace schema = svf::A0;

  auto ctx = svf::runtime::write_start<schema::Entry>(writer_fn, writer_ptr);

  schema::Target target0 = {
    .value = 0x1111111111111111ull,
  };

  schema::Entry entry = {
    .reference = svf::runtime::write_reference(&ctx, &target0),
    .someStruct = {
      .sequence = {},
      .someChoice_tag = schema::SomeChoice_tag::target,
      .someChoice_payload = {
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
    svf::runtime::write_sequence_element(&ctx, &target, &entry.someStruct.sequence);
  }

  svf::runtime::write_finish(&ctx, &entry);

  if (!ctx.finished) {
    terminate_with_message("Write was not finished.");
  }
}

void test_read(vm::LinearArena *arena, Bytes input_range) {
  namespace schema = svf::A1;

  auto scratch_memory = vm::many<U8>(
    arena,
    schema::_SchemaDescription::min_read_scratch_memory_size
  );
  auto read_result = svf::runtime::read_message<schema::Entry>(
    svf::runtime::Range<U8> { input_range.pointer, (U32) input_range.count },
    svf::runtime::Range<U8> { scratch_memory.pointer, (U32) scratch_memory.count },
    svf::runtime::CompatibilityLevel::compatibility_binary
  );

  if (!read_result.entry) {
    terminate_with_message("Read result is invalid.");
  }

  auto ctx = &read_result.context;
  auto entry = read_result.entry;

  auto target0 = svf::runtime::read_reference(ctx, entry->reference);
  ASSERT(target0);
  ASSERT(target0->value == 0x1111111111111111ull);

  ASSERT(entry->someStruct.sequence.count == 2);
  auto e0 = svf::runtime::read_sequence_element(ctx, entry->someStruct.sequence, 0);
  auto e1 = svf::runtime::read_sequence_element(ctx, entry->someStruct.sequence, 1);
  ASSERT(e0 && e1);
  ASSERT(e0->value == 0x2222222222222222ull);
  ASSERT(e1->value == 0x3333333333333333ull);

  ASSERT(entry->someStruct.someChoice_tag == schema::SomeChoice_tag::target);
  ASSERT(entry->someStruct.someChoice_payload.target.value == 0x4444444444444444ull);
}
