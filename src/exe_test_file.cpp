// UNREVIEWED.
#include <cstdio>
#include <cstring>
#include "utilities.hpp"
#include "platform.hpp"
#include "../meta/meta.hpp"
#include "../example/schema_a/schema_a.hpp"

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

namespace example_schema_binary {
  #include "../example/schema_a/schema_a.inc"

  Range<Byte> range = {
    .pointer = (Byte *) bytes,
    .count = sizeof(bytes),
  };
}

namespace meta = svf::svf_meta;
namespace example = svf::schema_a;

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

int test_write() {
  auto arena = vm::create_linear_arena(2ull < 30);
  if (!arena.reserved_range.pointer) {
    printf("Failed to create arena.\n");
    return 1;
  }

  auto header = vm::one<MessageHeader>(&arena);
  auto out_schema = vm::many<Byte>(&arena, example_schema_binary::range.count);
  auto root = vm::one<example::A>(&arena);

  memcpy(
    out_schema.pointer,
    example_schema_binary::range.pointer,
    example_schema_binary::range.count
  );

  *header = {
    .magic = { 'S', 'V', 'F', '\0' },
    .version = 0,
    .entry_name_hash = example::A_name_hash,
    .schema_offset = offset_between(header, out_schema.pointer),
    .schema_length = safe_int_cast<U32>(example_schema_binary::range.count),
    .data_offset = offset_between(header, root),
    .data_length = 0, // will be filled in later
  };

  auto structs = vm::allocate_many<meta::StructDefinition>(&arena, 1);
  ASSERT(structs.pointer); // temporary

  *root = {
    .left = 0x0123456789ABCDEFull,
    .right = 0xFEDCBA9876543210ull,
  };

  header->data_length = arena.waterline - header->data_offset;

  auto file = fopen(".tmp/test_file.svf", "wb");
  if (!file) {
    printf("Failed to open file for writing.\n");
    return 1;
  }

  {
    auto result = fwrite(arena.reserved_range.pointer, 1, arena.waterline, file);
    if (result != arena.waterline) {
      printf("Failed to write to file.\n");
      return 1;
    }
  }

  {
    auto result = fclose(file);
    if (result != 0) {
      printf("Failed to close file.\n");
      return 1;
    }
  }

  return 0;
}

int test_read() {
  auto file = fopen(".tmp/test_file.svf", "rb");
  if (!file) {
    printf("Failed to open file for reading.\n");
    return 1;
  }

  {
    auto result = fseek(file, 0, SEEK_END);
    if (result != 0) {
      printf("Failed to seek to end of file.\n");
      return 1;
    }
  }

  auto file_size = ftell(file);

  {
    auto result = fseek(file, 0, SEEK_SET);
    if (result != 0) {
      printf("Failed to seek to beginning of file.\n");
      return 1;
    }
  }

  auto arena = vm::create_linear_arena(2ull < 30);
  if (!arena.reserved_range.pointer) {
    printf("Failed to create arena.\n");
    return 1;
  }

  auto file_range = vm::allocate_many<Byte>(&arena, file_size);
  {
    auto result = fread(file_range.pointer, 1, file_range.count, file);
    if (result != file_size) {
      printf("Failed to read from file.\n");
      return 1;
    }
  }

  {
    auto result = fclose(file);
    if (result != 0) {
      printf("Failed to close file.\n");
      return 1;
    }
  }

  auto header = (MessageHeader *) file_range.pointer;
  if (
    header->magic[0] != 'S' ||
    header->magic[1] != 'V' ||
    header->magic[2] != 'F' ||
    header->magic[3] != '\0'
  ) {
    printf("File is not an SVF file.\n");
    return 1;
  }

  if (header->version != 0) {
    printf("File is not an SVF version 0 file.\n");
    return 1;
  }
  
  if (header->entry_name_hash != meta::Schema_name_hash) {
    printf("File does not contain the expected entry point.\n");
  }

  ASSERT(header->schema_offset + header->schema_length <= header->data_offset);
  auto schema_range = Range<Byte> {
    .pointer = file_range.pointer + header->schema_offset,
    .count = header->schema_length,
  };

#if 1
  // Check exact equality.
  ASSERT(schema_range.count == example_schema_binary::range.count);
  for (U64 i = 0; i < schema_range.count; i++) {
    if (schema_range.pointer[i] != example_schema_binary::range.pointer[i]) {
      printf("File does not contain the expected schema.\n");
      return 1;
    }
  }
#else
  ASSERT(example_schema_binary::range.count >= sizeof(meta::Schema));
  if (schema_range.count < sizeof(meta::Schema)) {
    printf("Schema embedded in file is too small.\n");
  }

  // Check binary compatibility.
  svf::compatiblity::binary::CheckContext check_context = {
    .r0 = meta_schema_binary::range,
    .r1 = schema_range,
    .s0 = (meta::Schema *) meta_schema_binary::range.pointer,
    .s1 = (meta::Schema *) schema_range.pointer,
  };

  auto result = svf::compatiblity::binary::check_entry(
    &check_context,
    meta::Schema_name_hash
  );
  if (!result) {
    printf("Schema in file is not binary compatible with expected schema.\n");
    return 1;
  }
#endif

  ASSERT(header->data_offset + header->data_length <= file_range.count);
  auto data_range = Range<Byte> {
    .pointer = file_range.pointer + header->data_offset,
    .count = header->data_length,
  };

  auto root = (example::A *) data_range.pointer;

  if (root->left != 0x0123456789ABCDEFull) {
    printf("root->left is not the expected value.\n");
    return 1;
  }

  if (root->right != 0xFEDCBA9876543210ull) {
    printf("root->right is not the expected value.\n");
    return 1;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  auto write_result = test_write();
  if (write_result != 0) {
    return write_result;
  }

  auto read_result = test_read();
  if (read_result != 0) {
    return read_result;
  }

  printf("All tests passed.\n");
  
  return 0;
}