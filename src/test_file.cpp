#include <cstdio>
#include <cstring>

#include "utilities.hpp"
#include "common.hpp"
#include "meta.hpp"

char const *example = R"(
EntryPoint: struct {
	i64: I64;
	f32: F32;
};
)";

struct Header {
	U8 magic[4];
	U8 version;
	U8 _reserved[3];
	U32 schema_offset;
	U32 entry_point_offset;
};

namespace meta_schema_binary {
	#include "meta.inc"

	Range<Byte> range = {
		.pointer = (Byte *) bytes,
		.count = sizeof(bytes),
	};
} // namespace meta_schema_binary

int test_write() {
	auto arena = vm::create_linear_arena(2ull < 30);
	if (!arena.reserved_range.pointer) {
		printf("Failed to create arena.\n");
		return 1;
	}

	auto header = vm::allocate_one<Header>(&arena);
	ASSERT(header); // temporary

	auto out_schema = vm::allocate_many<Byte>(&arena, meta_schema_binary::range.count);
	ASSERT(out_schema.pointer); // temporary
	memcpy(out_schema.pointer, meta_schema_binary::range.pointer, meta_schema_binary::range.count);

	auto entry_point = vm::allocate_one<svf::schema_format_0::EntryPoint>(&arena);
	ASSERT(entry_point); // temporary

	*header = {
		.magic = { 'S', 'V', 'F', '\0' },
		.version = 0,
		.schema_offset = offset_between(header, out_schema.pointer),
		.entry_point_offset = offset_between(header, entry_point),
	};

	auto structs = vm::allocate_many<svf::schema_format_0::StructDefinition>(&arena, 1);
	ASSERT(structs.pointer); // temporary

	*entry_point = {
		.structs = {
			.pointer_offset = offset_between(entry_point, structs.pointer),
			.count = safe_int_cast<U32>(structs.count),
		},
		.choices = {},
	};

	auto fields = vm::allocate_many<svf::schema_format_0::FieldDefinition>(&arena, 2);
	ASSERT(fields.pointer); // temporary

	structs.pointer[0] = {
		.fields = {
			.pointer_offset = offset_between(entry_point, fields.pointer),
			.count = safe_int_cast<U32>(fields.count),
		},
	};

	fields.pointer[0] = {
		.name_hash = compute_cstr_hash("i64"),
		.type_enum = svf::schema_format_0::Type_enum::concrete,
		.type_union = {
			.concrete = {
				.type_enum = svf::schema_format_0::ConcreteType_enum::i64,
			},
		},
	};

	fields.pointer[1] = {
		.name_hash = compute_cstr_hash("f32"),
		.type_enum = svf::schema_format_0::Type_enum::concrete,
		.type_union = {
			.concrete = {
				.type_enum = svf::schema_format_0::ConcreteType_enum::f32,
			},
		},
	};

	auto file = fopen("test_file.svf", "wb");
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
	auto file = fopen("test_file.svf", "rb");
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

	auto header = (Header *) file_range.pointer;
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

	auto schema_range = Range<Byte> {
		.pointer = file_range.pointer + header->schema_offset,
		.count = header->entry_point_offset - header->schema_offset,
	};

	ASSERT(schema_range.count == meta_schema_binary::range.count);
	for (U64 i = 0; i < schema_range.count; i++) {
		if (schema_range.pointer[i] != meta_schema_binary::range.pointer[i]) {
			printf("File does not contain the expected schema.\n");
			return 1;
		}
	}

	auto data_range = Range<Byte> {
		.pointer = file_range.pointer + header->entry_point_offset,
		.count = file_size - header->entry_point_offset,
	};

	auto entry_point = (svf::schema_format_0::EntryPoint *) data_range.pointer;

	auto structs_range = Range<svf::schema_format_0::StructDefinition> {
		.pointer = (svf::schema_format_0::StructDefinition *) (
			data_range.pointer + entry_point->structs.pointer_offset
		),
		.count = entry_point->structs.count,
	};

	if (structs_range.count != 1) {
		printf("File does not contain exactly one struct.\n");
		return 1;
	}

	auto struct_ = structs_range.pointer[0];

	auto fields_range = Range<svf::schema_format_0::FieldDefinition> {
		.pointer = (svf::schema_format_0::FieldDefinition *) (
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

	if (field_0.type_enum != svf::schema_format_0::Type_enum::concrete) {
		printf("First field does not have the expected type.\n");
		return 1;
	}

	if (field_0.type_union.concrete.type_enum != svf::schema_format_0::ConcreteType_enum::i64) {
		printf("First field does not have the expected concrete type.\n");
		return 1;
	}

	auto field_1 = fields_range.pointer[1];

	if (field_1.name_hash != compute_cstr_hash("f32")) {
		printf("Second field does not have the expected name.\n");
		return 1;
	}

	if (field_1.type_enum != svf::schema_format_0::Type_enum::concrete) {
		printf("Second field does not have the expected type.\n");
		return 1;
	}

	if (field_1.type_union.concrete.type_enum != svf::schema_format_0::ConcreteType_enum::f32) {
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