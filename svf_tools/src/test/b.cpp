#include <cstdio>
#include <cstdlib>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <generated/hpp/b0.hpp>
#include <generated/hpp/b1.hpp>
#include <src/svf_runtime.hpp>
#include <src/library.hpp>

void test_write(svf::runtime::WriterFn *writer_fn, void *writer_ptr) {
  namespace schema = svf::B0;

  auto ctx = svf::runtime::write_message_start<schema::Entry>(
    writer_ptr,
    writer_fn
  );

  schema::Entry entry = {
    .reorder_fields = {
      .one = 42,
      .two = 43,
    },
    .reorder_options_enum = schema::ReorderOptions_enum::one,
    .reorder_options_union = {
      .one = 44,
    },
    .remove_field = {
      .one = 45,
      .two = 46,
      .three = 47,
    },
    .add_option_enum = schema::AddOption_enum::three,
    .add_option_union = {
      .three = 48,
    },
    .primitives = {
      .u8_to_u16 = 49,
      .u8_to_u32 = 50,
      .u8_to_u64 = 51,
      .u8_to_i16 = 52,
      .u8_to_i32 = 53,
      .u8_to_i64 = 54,
      .u16_to_u32 = 55,
      .u16_to_u64 = 56,
      .u16_to_i32 = 57,
      .u16_to_i64 = 58,
      .u32_to_u64 = 59,
      .u32_to_i64 = 60,
      .i8_to_i16 = 61,
      .i8_to_i32 = 62,
      .i8_to_i64 = 63,
      .i16_to_i32 = 64,
      .i16_to_i64 = 65,
      .i32_to_i64 = 66,
      .f32_to_f64 = 67.0,
    },
  };

  svf::runtime::write_message_end(&ctx, &entry);

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

  vm::realign(arena);
  auto scratch_memory = vm::many<U8>(
    arena,
    schema::SchemaDescription::min_read_scratch_memory_size
  );
  auto read_result = svf::runtime::read_message<schema::Entry>(
    svf::runtime::Range<U8> { input_range.pointer, input_range.count },
    svf::runtime::Range<U8> { scratch_memory.pointer, scratch_memory.count },
    svf::runtime::CompatibilityLevel::compatibility_logical,
    malloc_adapter
  );

  if (!read_result.entry) {
    printf("Read result is invalid.\n");
    abort_this_process();
  }

  auto entry = read_result.entry;

  ASSERT(entry->reorder_fields.one == 42 && entry->reorder_fields.two == 43);
  ASSERT(entry->reorder_options_enum == schema::ReorderOptions_enum::one);
  ASSERT(entry->reorder_options_union.one == 44);
  ASSERT(entry->remove_field.one == 45 && entry->remove_field.three == 47);
  ASSERT(entry->add_option_enum == schema::AddOption_enum::three);

  ASSERT(entry->add_option_union.three == 48);
  ASSERT(entry->primitives.u8_to_u16 == 49);
  ASSERT(entry->primitives.u8_to_u32 == 50);
  ASSERT(entry->primitives.u8_to_u64 == 51);
  ASSERT(entry->primitives.u8_to_i16 == 52);
  ASSERT(entry->primitives.u8_to_i32 == 53);
  ASSERT(entry->primitives.u8_to_i64 == 54);
  ASSERT(entry->primitives.u16_to_u32 == 55);
  ASSERT(entry->primitives.u16_to_u64 == 56);
  ASSERT(entry->primitives.u16_to_i32 == 57);
  ASSERT(entry->primitives.u16_to_i64 == 58);
  ASSERT(entry->primitives.u32_to_u64 == 59);
  ASSERT(entry->primitives.u32_to_i64 == 60);
  ASSERT(entry->primitives.i8_to_i16 == 61);
  ASSERT(entry->primitives.i8_to_i32 == 62);
  ASSERT(entry->primitives.i8_to_i64 == 63);
  ASSERT(entry->primitives.i16_to_i32 == 64);
  ASSERT(entry->primitives.i16_to_i64 == 65);
  ASSERT(entry->primitives.i32_to_i64 == 66);
  ASSERT(entry->primitives.f32_to_f64 == 67.0);
}
