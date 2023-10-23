#undef NDEBUG
#include <assert.h>
#include <string.h>

#include <generated/h/JSON.h>
#include <src/svf_runtime.h>

void example_read(SVFRT_Bytes input_bytes) {
  uint8_t scratch_buffer[SVF_JSON_min_read_scratch_memory_size];
  SVFRT_Bytes scratch = { scratch_buffer, sizeof(scratch_buffer) };

  SVFRT_ReadMessageResult read_result;
  SVFRT_ReadMessageParams read_params;
  SVFRT_SET_DEFAULT_READ_PARAMS(&read_params, SVF_JSON, SVF_JSON_Item);

  SVFRT_read_message(&read_params, &read_result, input_bytes, scratch);
  assert(read_result.entry);

  SVFRT_ReadContext *ctx = &read_result.context;
  SVF_JSON_Item *entry = (SVF_JSON_Item *) read_result.entry;

  assert(entry->value_tag == SVF_JSON_Value_tag_object);
  assert(entry->value_payload.object.count == 2);

  SVF_JSON_Field const *field0 = SVFRT_READ_SEQUENCE_ELEMENT(SVF_JSON_Field, ctx, entry->value_payload.object, 0);
  SVF_JSON_Field const *field1 = SVFRT_READ_SEQUENCE_ELEMENT(SVF_JSON_Field, ctx, entry->value_payload.object, 1);
  assert(field0 && field1);

  uint8_t const *name0_ptr = SVFRT_READ_SEQUENCE_RAW(uint8_t, ctx, field0->name);
  uint8_t const *name1_ptr = SVFRT_READ_SEQUENCE_RAW(uint8_t, ctx, field1->name);
  assert(strncmp("hello", (char *) name0_ptr, field0->name.count) == 0);
  assert(strncmp("world", (char *) name1_ptr, field1->name.count) == 0);

  assert(field0->value_tag == SVF_JSON_Value_tag_number);
  assert(field0->value_payload.number == 42.0);

  assert(field1->value_tag == SVF_JSON_Value_tag_array);
  assert(field1->value_payload.array.count == 42);

  for (int i = 0; i < 42; i++) {
    SVF_JSON_Item const *item = SVFRT_READ_SEQUENCE_ELEMENT(SVF_JSON_Item, ctx, field1->value_payload.array, i);
    assert(item);
    assert(item->value_tag == SVF_JSON_Value_tag_number);
    assert(item->value_payload.number == i);
  }
}
