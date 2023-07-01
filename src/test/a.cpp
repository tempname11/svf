// UNREVIEWED.
#include <cstdio>
#include <cstring>
#include <generated/hpp/a0.hpp>
#include <generated/hpp/a1.hpp>
#include <generated/hpp/meta.hpp>
#include "../utilities.hpp"
#include "../platform.hpp"

struct MessageHeader {
  U8 magic[4];
  U8 version;
  U8 _reserved[3];
  U64 entry_name_hash;
  U32 schema_offset;
  U32 schema_length;
  U32 data_offset;
  U32 data_length;
};

namespace a0_binary {
  #include <generated/inc/a0.inc>

  Range<Byte> range = {
    .pointer = (Byte *) bytes,
    .count = sizeof(bytes),
  };
}

namespace a1_binary {
  #include <generated/inc/a1.inc>

  Range<Byte> range = {
    .pointer = (Byte *) bytes,
    .count = sizeof(bytes),
  };
}

namespace meta = svf::svf_meta;
namespace a0 = svf::schema_a0;
namespace a1 = svf::schema_a1;

namespace svf::compatiblity::binary {
  // Copied `from src/svf/compatibility.cpp`. TODO
  struct CheckContext {
    Range<Byte> r0;
    Range<Byte> r1;
    meta::Schema* s0;
    meta::Schema* s1;
  };

  Bool check_entry(
    CheckContext *ctx,
    U64 entry_name_hash
  );
}

Range<Byte> test_write(vm::LinearArena *arena) {
  auto header = vm::one<MessageHeader>(arena);
  auto out_schema = vm::many<Byte>(arena, a0_binary::range.count);
  auto root = vm::one<a0::A>(arena);

  *header = {
    .magic = { 'S', 'V', 'F', '\0' },
    .version = 0,
    .entry_name_hash = a0::A_name_hash,
    .schema_offset = offset_between(header, out_schema.pointer),
    .schema_length = safe_int_cast<U32>(a0_binary::range.count),
    .data_offset = offset_between(header, root),
    .data_length = 0, // Will be filled in later.
  };

  memcpy(
    out_schema.pointer,
    a0_binary::range.pointer,
    a0_binary::range.count
  );

  *root = {
    .left = 0x0123456789ABCDEFull,
    .right = 0xFEDCBA9876543210ull,
  };

  header->data_length = offset_between(root, vm::none<Byte>(arena));

  return Range<Byte> {
    .pointer = (Byte *) header,
    .count = offset_between(header, vm::none<Byte>(arena))
  };
}

void test_read(Range<Byte> input_range) {
  auto header = (MessageHeader *) input_range.pointer;
  if (
    header->magic[0] != 'S' ||
    header->magic[1] != 'V' ||
    header->magic[2] != 'F' ||
    header->magic[3] != '\0'
  ) {
    printf("File is not an SVF file.\n");
    abort_this_process();
  }

  if (header->version != 0) {
    printf("File is not an SVF version 0 file.\n");
    abort_this_process();
  }
  
  if (header->entry_name_hash != a1::A_name_hash) {
    printf("File does not contain the expected entry point.\n");
  }

  ASSERT(header->schema_offset + header->schema_length <= header->data_offset);
  auto schema_range = Range<Byte> {
    .pointer = input_range.pointer + header->schema_offset,
    .count = header->schema_length,
  };

#if 0
  // Check exact equality.
  ASSERT(schema_range.count == a1_binary::range.count);
  for (U64 i = 0; i < schema_range.count; i++) {
    if (schema_range.pointer[i] != a1_binary::range.pointer[i]) {
      printf("File does not contain the expected schema.\n");
      return 1;
    }
  }
#else
  ASSERT(a1_binary::range.count >= sizeof(meta::Schema));
  if (schema_range.count < sizeof(meta::Schema)) {
    printf("Schema embedded in file is too small.\n");
  }

  // Check binary compatibility.
  svf::compatiblity::binary::CheckContext check_context = {
    .r0 = a1_binary::range,
    .r1 = schema_range,
    .s0 = (meta::Schema *) a1_binary::range.pointer,
    .s1 = (meta::Schema *) schema_range.pointer,
  };

  auto result = svf::compatiblity::binary::check_entry(
    &check_context,
    a1::A_name_hash
  );
  if (!result) {
    printf("Schema in file is not binary compatible with expected schema.\n");
    abort_this_process();
  }
#endif

  ASSERT(header->data_offset + header->data_length <= input_range.count);
  auto data_range = Range<Byte> {
    .pointer = input_range.pointer + header->data_offset,
    .count = header->data_length,
  };

  auto root = (a1::A *) data_range.pointer;

  if (root->left != 0x0123456789ABCDEFull) {
    printf("root->left is not the expected value.\n");
    abort_this_process();
  }

  if (root->right != 0xFEDCBA9876543210ull) {
    printf("root->right is not the expected value.\n");
    abort_this_process();
  }
}

int main(int argc, char *argv[]) {
  auto arena_ = vm::create_linear_arena(2ull < 30);
  auto arena = &arena_;
  if (!arena->reserved_range.pointer) {
    printf("Failed to create arena.\n");
    abort_this_process();
  }

  auto write_result = test_write(arena);
  test_read(write_result);
  printf("Test passed.\n");
  return 0;
}