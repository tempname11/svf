// UNREVIEWED.
#include <cstdio>
#include <cstring>
#include "utilities.hpp"
#include "platform.hpp"
#include "meta.hpp"

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

namespace meta_schema_binary {
  #include "meta.inc"

  Range<Byte> range = {
    .pointer = (Byte *) bytes,
    .count = sizeof(bytes),
  };
}

namespace meta = svf::svf_meta;

namespace svf::compatiblity::binary {
  // Copied `from src/svf/compatibility.cpp`. TODO
  struct CheckContext {
    Range<Byte> r0;
    Range<Byte> r1;
    meta::Schema* s0;
    meta::Schema* s1;
  };

  bool check_entry(
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

  auto header = vm::allocate_one<MessageHeader>(&arena);
  ASSERT(header); // temporary

  auto out_schema = vm::allocate_many<Byte>(&arena, meta_schema_binary::range.count);
  ASSERT(out_schema.pointer); // temporary
  memcpy(out_schema.pointer, meta_schema_binary::range.pointer, meta_schema_binary::range.count);

  auto root = vm::allocate_one<meta::Schema>(&arena);
  ASSERT(root); // temporary

  *header = {
    .magic = { 'S', 'V', 'F', '\0' },
    .version = 0,
    .entry_name_hash = meta::Schema_name_hash,
    .schema_offset = offset_between(header, out_schema.pointer),
    .schema_length = safe_int_cast<U32>(meta_schema_binary::range.count),
    .data_offset = offset_between(header, root),
    .data_length = 0, // will be filled in later
  };

  auto structs = vm::allocate_many<meta::StructDefinition>(&arena, 1);
  ASSERT(structs.pointer); // temporary

  *root = {
    .structs = {
      .pointer_offset = offset_between(root, structs.pointer),
      .count = safe_int_cast<U32>(structs.count),
    },
    .choices = {},
  };

  auto fields = vm::allocate_many<meta::FieldDefinition>(&arena, 2);
  ASSERT(fields.pointer); // temporary

  structs.pointer[0] = {
    .fields = {
      .pointer_offset = offset_between(root, fields.pointer),
      .count = safe_int_cast<U32>(fields.count),
    },
  };

  fields.pointer[0] = {
    .name_hash = compute_cstr_hash("i64"),
    .type_enum = meta::Type_enum::concrete,
    .type_union = {
      .concrete = {
        .type_enum = meta::ConcreteType_enum::i64,
      },
    },
  };

  fields.pointer[1] = {
    .name_hash = compute_cstr_hash("f32"),
    .type_enum = meta::Type_enum::concrete,
    .type_union = {
      .concrete = {
        .type_enum = meta::ConcreteType_enum::f32,
      },
    },
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

#if 0
  // Check exact equality.
  ASSERT(schema_range.count == meta_schema_binary::range.count);
  for (U64 i = 0; i < schema_range.count; i++) {
    if (schema_range.pointer[i] != meta_schema_binary::range.pointer[i]) {
      printf("File does not contain the expected schema.\n");
      return 1;
    }
  }
#else
  ASSERT(meta_schema_binary::range.count >= sizeof(meta::Schema));
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

  auto result = svf::compatiblity::binary::check_entry(&check_context, meta::Schema_name_hash);
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

  auto root = (meta::Schema *) data_range.pointer;

  auto structs_range = Range<meta::StructDefinition> {
    .pointer = (meta::StructDefinition *) (
      data_range.pointer + root->structs.pointer_offset
    ),
    .count = root->structs.count,
  };

  if (structs_range.count != 1) {
    printf("File does not contain exactly one struct.\n");
    return 1;
  }

  auto struct_ = structs_range.pointer[0];

  auto fields_range = Range<meta::FieldDefinition> {
    .pointer = (meta::FieldDefinition *) (
      data_range.pointer + struct_.fields.pointer_offset
    ),
    .count = struct_.fields.count,
  };

  if (fields_range.count != 2) {
    printf("Struct does not contain exactly two fields.\n");
    return 1;
  }

  auto field_0 = fields_range.pointer[0];

  if (field_0.name_hash != compute_cstr_hash("i64")) {
    printf("First field does not have the expected name.\n");
    return 1;
  }

  if (field_0.type_enum != meta::Type_enum::concrete) {
    printf("First field does not have the expected type.\n");
    return 1;
  }

  if (field_0.type_union.concrete.type_enum != meta::ConcreteType_enum::i64) {
    printf("First field does not have the expected concrete type.\n");
    return 1;
  }

  auto field_1 = fields_range.pointer[1];

  if (field_1.name_hash != compute_cstr_hash("f32")) {
    printf("Second field does not have the expected name.\n");
    return 1;
  }

  if (field_1.type_enum != meta::Type_enum::concrete) {
    printf("Second field does not have the expected type.\n");
    return 1;
  }

  if (field_1.type_union.concrete.type_enum != meta::ConcreteType_enum::f32) {
    printf("Second field does not have the expected concrete type.\n");
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