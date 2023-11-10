#undef NDEBUG
#include <cassert>

#include <generated/hpp/JSON.hpp>
#include <src/svf_runtime.hpp>
#include <src/svf_stdio.h>

// Writes the equivalent of:
//
// {"hello": 42, "world": [0, 1, ..., 41]}
void example_write(FILE *file) {
  auto ctx = svf::runtime::write_start<svf::JSON::Item>(SVFRT_fwrite, (void *) file);

  svf::JSON::Field fields[2] = {};

  fields[0].name = svf::runtime::write_fixed_size_string<uint8_t>(&ctx, "hello", 0);
  fields[0].value_tag = svf::JSON::Value_tag::number;
  fields[0].value_payload.number = 42.0;

  fields[1].name = svf::runtime::write_fixed_size_string<uint8_t>(&ctx, "world", 0);
  fields[1].value_tag = svf::JSON::Value_tag::array;
  for (int i = 0; i < 42; i++) {
    svf::JSON::Item item = {};
    item.value_tag = svf::JSON::Value_tag::number;
    item.value_payload.number = i;
    svf::runtime::write_sequence_element(&ctx, &item, &fields[1].value_payload.array);
  }

  svf::JSON::Item entry = {};
  entry.value_tag = svf::JSON::Value_tag::object;
  entry.value_payload.object = svf::runtime::write_fixed_size_array(&ctx, fields);

  svf::runtime::write_finish(&ctx, &entry);
  assert(ctx.finished);
}
