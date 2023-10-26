#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <generated/hpp/B0.hpp>
#include <generated/hpp/B1.hpp>
#include <src/svf_runtime.hpp>
#include <src/library.hpp>

void test_write(svf::runtime::WriterFn *writer_fn, void *writer_ptr) {
  namespace schema = svf::B0;

  auto ctx = svf::runtime::write_start<schema::Entry>(writer_fn, writer_ptr);

  schema::Entry entry = {
    .reorderFields = {
      .one = 42,
      .two = 43,
    },
    .reorderOptions_tag = schema::ReorderOptions_tag::one,
    .reorderOptions_payload = {
      .one = 44,
    },
    .removeField = {
      .one = 45,
      .two = 46,
      .three = 47,
    },
    .addOption_tag = schema::AddOption_tag::three,
    .addOption_payload = {
      .three = 48,
    },
    .primitives = {
      .u8u16 = 49,
      .u8u32 = 50,
      .u8u64 = 51,
      .u8i16 = 52,
      .u8i32 = 53,
      .u8i64 = 54,
      .u16u32 = 55,
      .u16u64 = 56,
      .u16i32 = 57,
      .u16i64 = 58,
      .u32u64 = 59,
      .u32i64 = 60,
      .i8i16 = 61,
      .i8i32 = 62,
      .i8i64 = 63,
      .i16i32 = 64,
      .i16i64 = 65,
      .i32i64 = 66,
      .f32f64 = 67.0,
    },
  };

  svf::runtime::write_finish(&ctx, &entry);

  if (!ctx.finished) {
    printf("Write was not finished.\n");
    abort_this_process();
  }
}

void *malloc_adapter(void */*unused*/, size_t size) {
  return malloc(size);
}

void test_read(vm::LinearArena *arena, Bytes input_range) {
  namespace schema = svf::B1;

  auto scratch_memory = vm::many<U8>(
    arena,
    schema::_SchemaDescription::min_read_scratch_memory_size
  );
  auto read_result = svf::runtime::read_message<schema::Entry>(
    svf::runtime::Range<U8> { input_range.pointer, (U32) input_range.count },
    svf::runtime::Range<U8> { scratch_memory.pointer, (U32) scratch_memory.count },
    svf::runtime::CompatibilityLevel::compatibility_logical,
    malloc_adapter
  );

  if (!read_result.entry) {
    printf("Read result is invalid, code=%" PRIx32 ".\n", read_result.error_code);
    abort_this_process();
  }

  auto entry = read_result.entry;

  ASSERT(entry->reorderFields.one == 42 && entry->reorderFields.two == 43);

  ASSERT(entry->reorderOptions_tag == schema::ReorderOptions_tag::one);
  ASSERT(entry->reorderOptions_payload.one == 44);

  ASSERT(entry->removeField.one == 45 && entry->removeField.three == 47);

  ASSERT(entry->addOption_tag == schema::AddOption_tag::three);
  ASSERT(entry->addOption_payload.three == 48);

  ASSERT(entry->primitives.u8u16 == 49);
  ASSERT(entry->primitives.u8u32 == 50);
  ASSERT(entry->primitives.u8u64 == 51);
  ASSERT(entry->primitives.u8i16 == 52);
  ASSERT(entry->primitives.u8i32 == 53);
  ASSERT(entry->primitives.u8i64 == 54);
  ASSERT(entry->primitives.u16u32 == 55);
  ASSERT(entry->primitives.u16u64 == 56);
  ASSERT(entry->primitives.u16i32 == 57);
  ASSERT(entry->primitives.u16i64 == 58);
  ASSERT(entry->primitives.u32u64 == 59);
  ASSERT(entry->primitives.u32i64 == 60);
  ASSERT(entry->primitives.i8i16 == 61);
  ASSERT(entry->primitives.i8i32 == 62);
  ASSERT(entry->primitives.i8i64 == 63);
  ASSERT(entry->primitives.i16i32 == 64);
  ASSERT(entry->primitives.i16i64 == 65);
  ASSERT(entry->primitives.i32i64 == 66);
  ASSERT(entry->primitives.f32f64 == 67.0);
}
