#include <stdio.h>
#include <stdlib.h>
#undef NDEBUG // For the test in Release mode.
#include <assert.h>

#define SVF_IMPLEMENTATION
#include <generated/svf.h>
#include <generated/h/A0.h>
#include <generated/h/A1.h>

void test_write(SVFRT_WriterFn *writer_fn, void *writer_ptr) {
  SVFRT_WriteContext ctx = {0};
  SVFRT_WRITE_START(SVF_A0, SVF_A0_Entry, &ctx, writer_fn, writer_ptr);

  SVF_A0_Target target0 = {
    .value = 0x1111111111111111ull,
  };

  SVF_A0_Entry entry = {
    .reference = SVFRT_WRITE_REFERENCE(&ctx, &target0),
    .someStruct = {
      .sequence = {0},
      .someChoice_enum = SVF_A0_SomeChoice_target,
      .someChoice_union = {
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
    SVFRT_WRITE_SEQUENCE_ELEMENT(&ctx, &target, &entry.someStruct.sequence);
  }

  SVFRT_WRITE_FINISH(&ctx, &entry);
  assert(ctx.finished);
}

void test_read(SVFRT_Bytes input_bytes) {
  char scratch_buffer[SVF_A1_min_read_scratch_memory_size];
  SVFRT_Bytes scratch = { (uint8_t *) scratch_buffer, sizeof(scratch_buffer) };

  SVFRT_ReadMessageResult read_result;
  SVFRT_ReadMessageParams read_params;
  SVFRT_SET_DEFAULT_READ_PARAMS(&read_params, SVF_A1, SVF_A1_Entry);

  SVFRT_read_message(&read_params, &read_result, input_bytes, scratch);
  assert(read_result.entry);

  SVFRT_ReadContext *ctx = &read_result.context;
  SVF_A1_Entry const *entry = read_result.entry;

  SVF_A1_Target const *target0 = SVFRT_READ_REFERENCE(SVF_A1_Target, ctx, entry->reference);
  assert(target0);
  assert(target0->value == 0x1111111111111111ull);

  assert(entry->someStruct.sequence.count == 2);
  SVF_A1_Target const *e0 = SVFRT_READ_SEQUENCE_ELEMENT(SVF_A1_Target, ctx, entry->someStruct.sequence, 0);
  SVF_A1_Target const *e1 = SVFRT_READ_SEQUENCE_ELEMENT(SVF_A1_Target, ctx, entry->someStruct.sequence, 1);
  assert(e0 && e1);
  assert(e0->value == 0x2222222222222222ull);
  assert(e1->value == 0x3333333333333333ull);

  assert(entry->someStruct.someChoice_enum == SVF_A1_SomeChoice_target);
  assert(entry->someStruct.someChoice_union.target.value == 0x4444444444444444ull);
}

uint32_t file_writer(void *writer_ptr, SVFRT_Bytes data) {
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
    test_read((SVFRT_Bytes) { pointer, size });
    assert(ferror(file) == 0);
    assert(fclose(file) == 0);
  }

  return 0;
}
