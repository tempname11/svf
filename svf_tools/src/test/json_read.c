#undef NDEBUG
#include <assert.h>
#include <string.h>

#include <generated/h/JSON.h>
#include <src/svf_runtime.h>

void example_read(SVFRT_Bytes bytes) {
  uint8_t scratch_buffer[SVF_JSON_min_read_scratch_memory_size];
  SVFRT_Bytes scratch = { scratch_buffer, sizeof(scratch_buffer) };
  SVFRT_ReadMessageResult result = {0};

  SVFRT_READ_MESSAGE(SVF_JSON, SVF_JSON_Item, &result, bytes, scratch, SVFRT_compatibility_binary, NULL, NULL);
  assert(result.entry);

  SVFRT_ReadContext *ctx = &result.context;
  SVF_JSON_Item *entry = result.entry;

  assert(entry->value_enum == SVF_JSON_Value_object);
  assert(entry->value_union.object.count == 2);

  SVF_JSON_Field const *field0 = SVFRT_READ_ARRAY_ELEMENT(SVF_JSON_Field, ctx, entry->value_union.object, 0);
  SVF_JSON_Field const *field1 = SVFRT_READ_ARRAY_ELEMENT(SVF_JSON_Field, ctx, entry->value_union.object, 1);
  assert(field0 && field1);

  uint8_t const *name0_ptr = SVFRT_READ_ARRAY_RAW(uint8_t, ctx, field0->name);
  uint8_t const *name1_ptr = SVFRT_READ_ARRAY_RAW(uint8_t, ctx, field1->name);
  assert(strncmp("hello", (char *) name0_ptr, field0->name.count) == 0);
  assert(strncmp("world", (char *) name1_ptr, field1->name.count) == 0);

  assert(field0->value_enum == SVF_JSON_Value_number);
  assert(field0->value_union.number == 42.0);

  assert(field1->value_enum == SVF_JSON_Value_array);
  assert(field1->value_union.array.count == 42);

  for (int i = 0; i < 42; i++) {
    SVF_JSON_Item const *item = SVFRT_READ_ARRAY_ELEMENT(SVF_JSON_Item, ctx, field1->value_union.array, i);
    assert(item);
    assert(item->value_enum == SVF_JSON_Value_number);
    assert(item->value_union.number == i);
  }
}