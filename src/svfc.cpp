#include <cstring>
#include "common.hpp"

char const input_cstr[] = R"#(
EntryPoint: struct {
  as: A[];
  bs: B[];
};

A: struct {
  fields: U32[];
};

B: struct {
  fields: U64[];
};

)#";
/*
char const input_cstr[] = R"#(
EntryPoint: struct {
  structs: StructDefinition[];
  choices: ChoiceDefinition[];
};

ChoiceDefinition: struct {
  options: OptionDefinition[];
};

OptionDefinition: struct {
  name_hash: U64;
  index: U8;
  type: Type;
};

StructDefinition: struct {
  fields: FieldDefinition[];
};

FieldDefinition: struct {
  name_hash: U64;
  type: Type;
};

ConcreteType: choice {
  u8;
  u16;
  u32;
  u64;
  i8;
  i16;
  i32;
  i64;
  f32;
  f64;
  defined: struct {
    top_level_definition_index: U32;
  };
};

Type: choice {
  concrete: struct {
    type: ConcreteType;
  };
  pointer: struct {
    type: ConcreteType;
  };
  fixed_size_array: struct {
    count: U8;
    element_type: ConcreteType;
  };
  flexible_array: struct {
    element_type: ConcreteType;
  };
};
)#";
*/

namespace grammar {

struct ConcreteType {
	enum class Which {
		u8,
		u16,
		u32,
		u64,
		i8,
		i16,
		i32,
		i64,
		f32,
		f64,
		defined,
		unresolved,
	} which;

	struct Defined {
		U32 top_level_definition_index;
	};

	struct Unresolved {
		Range<U8> name;
	};
	// This is only necessary before everything was parsed.
	// After that, a second pass must resolve all names.

	union {
		Defined defined;
		Unresolved unresolved;
	};
};

struct Type {
	enum class Which {
		concrete,
		pointer,
		fixed_size_array,
		flexible_array,
	} which;

	struct Concrete {
		ConcreteType type;
	};

	struct Pointer {
		ConcreteType type;
	};

	struct FixedSizeArray {
		U8 count;
		ConcreteType element_type;
	};

	struct FlexibleArray {
		ConcreteType element_type;
	};

	union {
		Concrete concrete;
		Pointer pointer;
	};
};

struct FieldDefinition {
	U64 name_hash;
	Type type;
};

struct OptionDefinition {
	U64 name_hash;
	U8 index;
	Type type;
};

struct StructDefinition {
	U64 name_hash;
	Range<FieldDefinition> fields;
};

struct ChoiceDefinition {
	U64 name_hash;
	Range<OptionDefinition> options;
};

struct TopLevelDefinition {
	enum class Which {
		a_struct,
		a_choice,
	} which;

	union {
		StructDefinition a_struct;
		ChoiceDefinition a_choice;
	};
};

struct Root {
	Range<StructDefinition> structs;
	Range<ChoiceDefinition> choices;
};

struct ParserState {
	U64 cursor;
	Bool fail;
};

struct ParserContext {
	vm::LinearArena *arena;
	Range<U8> input;
	ParserState state;
};

using Ctx = ParserContext *;

void skip_whitespace(Ctx ctx) {
	while (true) {
		if (ctx->state.fail || ctx->state.cursor == ctx->input.count) {
			return;
		}

		auto byte = ctx->input.pointer[ctx->state.cursor];

		if (byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n') {
			ctx->state.cursor++;
		} else {
		  break;
		}
	}
}

Range<U8> parse_name(Ctx ctx, Bool is_type) {
	size_t cursor_start = ctx->state.cursor;
	if (ctx->state.fail || ctx->state.cursor == ctx->input.count) {
		ctx->state.fail = 1;
		return {};
	}

	{
		auto byte = ctx->input.pointer[ctx->state.cursor];

		if (0
			|| (is_type && byte >= 'A' && byte <= 'Z')
			|| (!is_type && byte >= 'a' && byte <= 'z')
		) {
			ctx->state.cursor++;
		} else {
			ctx->state.fail = 1;
			return {};
		}
	}

	while (true) {
		if (ctx->state.cursor == ctx->input.count) {
			break;
		}

		auto byte = ctx->input.pointer[ctx->state.cursor];

		if (0
		  || (byte >= 'A' && byte <= 'Z')
		  || (byte >= 'a' && byte <= 'z')
		  || (byte >= '0' && byte <= '9')
			|| byte == '_'
		) {
			ctx->state.cursor++;
		} else {
			break;
		}
	}

	return Range<U8> {
		.pointer = (ctx->input.pointer + cursor_start),
		.count = ctx->state.cursor - cursor_start,
	};
}

Range<U8> parse_type_name(Ctx ctx) { return parse_name(ctx, true); }
Range<U8> parse_value_name(Ctx ctx) { return parse_name(ctx, false); }

void skip_specific_character(Ctx ctx, U8 ascii_character) {
	if (ctx->state.fail || ctx->state.cursor == ctx->input.count) {
		ctx->state.fail = 1;
		return;
	}

	auto byte = ctx->input.pointer[ctx->state.cursor];

	if (byte == ascii_character) {
		ctx->state.cursor++;
	} else {
		ctx->state.fail = 1;
	}
}

void skip_specific_cstring(Ctx ctx, char const* cstr) {
	auto clen = strlen(cstr);
	if (ctx->state.cursor + clen > ctx->input.count) {
		ctx->state.fail = 1;
		return;
	}

	for (size_t i = 0; i < clen; i++) {
		if (ctx->input.pointer[ctx->state.cursor + i] != cstr[i]) {
			ctx->state.fail = 1;
		}
	}

	ctx->state.cursor += clen;
}

Type parse_type(Ctx ctx) {
	auto name = parse_type_name(ctx);
	skip_whitespace(ctx);

	if (ctx->state.fail || ctx->state.cursor == ctx->input.count) {
		ctx->state.fail = 1;
		return {};
	}

	auto byte = ctx->input.pointer[ctx->state.cursor];

	if (byte == '*') {
		return {
			.which = Type::Which::pointer,
			.pointer = {
				.type = {
					.which = ConcreteType::Which::unresolved,
					.unresolved = {
						.name = name,
					},
				},
			},
		};
	} else if (byte == '[') {
		ctx->state.cursor++;
		skip_whitespace(ctx);
		skip_specific_character(ctx, ']'); // TODO: fixed size array
		return {
			.which = Type::Which::flexible_array,
			.pointer = {
				.type = {
					.which = ConcreteType::Which::unresolved,
					.unresolved = {
						.name = name,
					},
				},
			},
		};
	} else {
		ctx->state.fail = 1;
		return {};
	}
}

void parse_choice(Ctx ctx, ChoiceDefinition *result) {
	auto definition_name = parse_type_name(ctx);
	skip_specific_character(ctx, ':');
	skip_whitespace(ctx);

	ASSERT(false);
	// TODO: probably mostly same as `parse_struct`
	// Need to review `parse_struct` again before copying it.
}

U64 compute_name_hash(Range<U8> name) {
	auto hash = hash64::begin();
	hash64::add_bytes(&hash, name);
	return hash;
}

void parse_struct(Ctx ctx, StructDefinition *result) {
	auto definition_name = parse_type_name(ctx);
	skip_specific_character(ctx, ':');
	skip_whitespace(ctx);
	skip_specific_cstring(ctx, "struct");
	skip_whitespace(ctx);
	skip_specific_character(ctx, '{');
	skip_whitespace(ctx);

	if (ctx->state.fail) {
		return;
	}

	DynamicArray<FieldDefinition> dynamic_fields = {};

	while (true) {
		auto saved_state = ctx->state;
		auto field_name = parse_value_name(ctx);
		result->name_hash = compute_name_hash(field_name);
		skip_specific_character(ctx, ':');
		skip_whitespace(ctx);
		auto type = parse_type(ctx);
		skip_whitespace(ctx);
		skip_specific_character(ctx, ';');
		skip_whitespace(ctx);

		if (ctx->state.fail) {
			ctx->state = saved_state;
			break;
		} else {
			dynamic_resize(&dynamic_fields, ctx->arena, dynamic_fields.count + 1);
			ASSERT(dynamic_fields.pointer); // temporary
			dynamic_fields.pointer[dynamic_fields.count++] = FieldDefinition {
				.name_hash = compute_name_hash(field_name),
				.type = type,
			};
		}
	}

	result->fields = {
		.pointer = dynamic_fields.pointer,
		.count = dynamic_fields.count,
	};

	skip_whitespace(ctx);
	skip_specific_character(ctx, '}');
	skip_whitespace(ctx);
	skip_specific_character(ctx, ';');
}

TopLevelDefinition *parse_top_level_definition(Ctx ctx) {
	auto result = vm::allocate_one<TopLevelDefinition>(ctx->arena);
	ASSERT(result); // temporary

	auto saved_state = ctx->state;
	parse_struct(ctx, &result->a_struct);

	if (!ctx->state.fail) {
		result->which = TopLevelDefinition::Which::a_struct;
		return result;
	}

	ctx->state = saved_state;
	parse_choice(ctx, &result->a_choice);

	if (!ctx->state.fail) {
		result->which = TopLevelDefinition::Which::a_choice;
		return result;
	}

	return 0;
}

Root *parse_input(vm::LinearArena *arena, Range<U8> input) {
	ParserContext context_value = {
		.arena = arena,
		.input = input,
		.state = {},
	};
	auto ctx = &context_value;
	auto result = vm::allocate_one<Root>(ctx->arena);
	ASSERT(result); // temporary

	skip_whitespace(ctx);
	while (true) {
		if (ctx->state.cursor == ctx->input.count) {
			break;
		}
		parse_top_level_definition(ctx);
		skip_whitespace(ctx);
		if (ctx->state.fail) {
			return 0;
		}
	}

	return result;
}

} // namespace grammar

int main(int argc, char *argv[]) {
	auto arena = vm::create_linear_arena(2ull << 30);
	// never free, we will just exit the program.

	if (!arena.reserved_range.pointer) {
		printf("Error: could not create main memory arena.\n");
		return 1;
	}

	static_assert(sizeof(char) == 1); // sanity check
	auto input = Range<U8> {
		.pointer = (U8 *) input_cstr,
		.count = strlen(input_cstr),
	};

	auto result = grammar::parse_input(&arena, input);

	printf("Success.\n");
	return 0;
}