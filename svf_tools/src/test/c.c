#include <stdio.h>
#include <stdlib.h>
#undef NDEBUG // For the test in Release mode.
#include <assert.h>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.h>
#include <generated/h/a0.h>
#include <generated/h/a1.h>

void test_write(SVFRT_WriterFn *writer_fn, void *writer_ptr) {
  SVFRT_WriteContext ctx = {0};
  SVFRT_WRITE_MESSAGE_START(
    SVF_A0,
    SVF_A0_Entry,
    &ctx,
    writer_fn,
    writer_ptr
  );

  SVF_A0_Target target0 = {
    .value = 0x1111111111111111ull,
  };

  SVF_A0_Entry entry = {
    .pointer = SVFRT_WRITE_POINTER(&ctx, &target0),
    .some_struct = {
      .array = {0},
      .some_choice_enum = SVF_A0_SomeChoice_target,
      .some_choice_union = {
        .target = {
          .value = 0x4444444444444444ull,
        },
      },
    },
  };

  for (size_t i = 0; i < 2; i++) {
    SVF_A0_Target target = {
      .value = (i + 2) * 0x1111111111111111ull,
    };
    SVFRT_WRITE_ARRAY_ELEMENT(&ctx, &target, &entry.some_struct.array);
  }

  SVFRT_WRITE_MESSAGE_END(&ctx, &entry);

  assert(ctx.finished);
}

void test_read(SVFRT_RangeU8 input_bytes) {
  uint8_t scratch_buffer[SVF_A1_min_read_scratch_memory_size];
  SVFRT_RangeU8 scratch_memory = { scratch_buffer, sizeof(scratch_buffer) };

  SVFRT_ReadMessageResult read_result = {0};
  SVFRT_READ_MESSAGE(
    SVF_A1,
    SVF_A1_Entry,
    &read_result,
    input_bytes,
    scratch_memory,
    SVFRT_compatibility_binary,
    NULL,
    NULL
  );

  assert(read_result.entry);

  SVFRT_ReadContext *ctx = &read_result.context;
  SVF_A1_Entry *entry = read_result.entry;

  SVF_A1_Target *target0 = SVFRT_READ_POINTER(SVF_A1_Target, ctx, entry->pointer);
  assert(target0);
  assert(target0->value == 0x1111111111111111ull);

  assert(entry->some_struct.array.count == 2);
  SVF_A1_Target *e0 = SVFRT_READ_ARRAY(SVF_A1_Target, ctx, entry->some_struct.array, 0);
  SVF_A1_Target *e1 = SVFRT_READ_ARRAY(SVF_A1_Target, ctx, entry->some_struct.array, 1);
  assert(e0 && e1);
  assert(e0->value == 0x2222222222222222ull);
  assert(e1->value == 0x3333333333333333ull);

  assert(entry->some_struct.some_choice_enum == SVF_A1_SomeChoice_target);
  assert(entry->some_struct.some_choice_union.target.value == 0x4444444444444444ull);
}

uint32_t file_writer(void *writer_ptr, SVFRT_RangeU8 data) {
  FILE *file = (FILE *) writer_ptr;
  size_t result = fwrite((void *) data.pointer, 1, data.count, file);
  return (uint32_t) result;
}

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;

  // Write.
  {
    FILE *file = fopen("tmp_c.svf", "wb");
    assert(file);
    test_write(file_writer, (void *) file);
    assert(ferror(file) == 0);
    assert(fclose(file) == 0);
  }

  // Read.
  {
    FILE *file = fopen("tmp_c.svf", "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    void *pointer = malloc(size);
    assert((size_t) size == fread(pointer, 1, size, file));
    test_read((SVFRT_RangeU8) { pointer, size });
    assert(ferror(file) == 0);
    assert(fclose(file) == 0);
  }
}
