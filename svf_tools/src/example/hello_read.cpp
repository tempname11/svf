#undef NDEBUG
#include <cassert>
#include <cstring>

#include <generated/hpp/Hello.hpp>
#include <src/svf_runtime.hpp>

void example_read(svf::runtime::Bytes bytes) {
  namespace RT = svf::runtime;
  namespace Hello = svf::Hello;

  uint8_t scratch_buffer[Hello::SchemaDescription::min_read_scratch_memory_size];
  RT::Range<uint8_t> scratch = { scratch_buffer, sizeof(scratch_buffer) };

  auto read_result = RT::read_message<Hello::World>(bytes, scratch, RT::CompatibilityLevel::compatibility_binary);
  auto world = read_result.entry;
  auto ctx = &read_result.context;

  assert(world);
  assert(world->population == 8000000000);
  assert(world->gravitationalConstant == 6.674e-11f);
  assert(world->currentYear == 2023);
  assert(world->mechanics_tag == Hello::Mechanics_tag::quantum);

  auto name = RT::read_sequence_raw(ctx, world->name.utf8);
  assert(strncmp("The Universe", (char const *) name.pointer, name.count) == 0);
}
