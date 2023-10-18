#undef NDEBUG
#include <cassert>

#include <generated/hpp/JSON.hpp>
#include <src/svf_runtime.hpp>
#include <src/svf_stdio.h>

// Writes the equivalent of:
//
// {"hello": 42, "world": [0, 1, ..., 41]}
void example_write(FILE *file) {
  namespace RT = svf::runtime;
  namespace JSON = svf::JSON;
  auto ctx = RT::write_start<JSON::Item>((void *) file, SVFRT_fwrite);

  JSON::Field fields[2] = {};

  // TODO: null-termination is not what we want.
  fields[0].name = RT::write_fixed_size_string<svf::U8>(&ctx, "hello");
  fields[0].value_enum = JSON::Value_enum::number;
  fields[0].value_union.number = 42.0;

  // TODO: null-termination is not what we want.
  fields[1].name = RT::write_fixed_size_string<svf::U8>(&ctx, "world");
  fields[1].value_enum = JSON::Value_enum::array;
  for (int i = 0; i < 42; i++) {
    JSON::Item item = {};
    item.value_enum = JSON::Value_enum::number;
    item.value_union.number = i;
    RT::write_sequence_element(&ctx, &item, &fields[1].value_union.array);
  }

  JSON::Item entry = {};
  entry.value_enum = JSON::Value_enum::object;
  entry.value_union.object = RT::write_fixed_size_array(&ctx, fields);

  svf::runtime::write_finish(&ctx, &entry);
  assert(ctx.finished);
}
