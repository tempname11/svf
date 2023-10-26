#undef NDEBUG
#include <cassert>
#include <cstring>

#include <generated/hpp/Hello.hpp>
#include <src/svf_runtime.hpp>

void example_read(svf::runtime::Bytes bytes) {
  uint8_t scratch_buffer[svf::Hello::_SchemaDescription::min_read_scratch_memory_size];
  auto read_result = svf::runtime::read_message<svf::Hello::World>(
    bytes,
    { scratch_buffer, sizeof(scratch_buffer) },
    svf::runtime::CompatibilityLevel::compatibility_binary
  );
  auto ctx = &read_result.context;
  auto world = read_result.entry;

  assert(world);
  assert(world->population == 8000000000);
  assert(world->gravitationalConstant == 6.674e-11f);
  assert(world->currentYear == 2023);
  assert(world->mechanics_tag == svf::Hello::Mechanics_tag::quantum);

  auto name = svf::runtime::read_sequence_raw(ctx, world->name.utf8);
  assert(name.count == 12);
  assert(strncmp("The Universe", (char const *) name.pointer, name.count) == 0);
}
