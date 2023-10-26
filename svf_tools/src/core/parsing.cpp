#include <cctype>
#include <cstdlib>
#include <src/library.hpp>
#include "../core.hpp"

namespace core::parsing {

using namespace grammar;

// TODO: check all the name hashes to not be zero. This is perhaps better suited
// for validation than for parsing, but needs to be done anyway.

// General note: the functions here try to parse optimistically, and only
// check failure when necessary. This leads to simpler, more straightforward code.

// Simple growable array specifically for the parser.
template<typename T>
struct CrudeDynamicArray {
  T* pointer;
  UInt count;
  UInt capacity;
};

// Makes sure that the array has enough capacity to hold the required count.
// Preserves the contents of the old array even on reallocation.
template<typename T>
static inline
void ensure_size(
  CrudeDynamicArray<T> *dynamic,
  vm::LinearArena *arena,
  UInt required_count
) {
  // If capacity is already enough, do nothing.
  if (dynamic->capacity >= required_count) {
    return;
  }

  UInt final_count = round_up_to_power_of_two(required_count);
  auto reserved_range = many<T>(arena, final_count);
  if (dynamic->count != 0) {
    memcpy(reserved_range.pointer, dynamic->pointer, dynamic->count * sizeof(T));
  }

  *dynamic = {
    .pointer = reserved_range.pointer,
    .count = dynamic->count,
    .capacity = reserved_range.count,
  };
}

template<typename T>
static inline
void ensure_one_more(
  CrudeDynamicArray<T> *dynamic,
  vm::LinearArena *arena
) {
  ensure_size(dynamic, arena, dynamic->count + 1);
}

Range<U8> get_fail_code_description(FailCode code) {
  switch(code) {
    case FailCode::expected_newline: {
      return range_from_cstr("Expected a newline.");
    }
    case FailCode::expected_name_directive: {
      return range_from_cstr("Expected a #name directive.");
    }
    case FailCode::expected_uppercase_letter: {
      return range_from_cstr("Expected an uppercase [A-Z] letter.");
    }
    case FailCode::expected_lowercase_letter: {
      return range_from_cstr("Expected a lowercase [a-z] letter.");
    }
    case FailCode::expected_type_definition: {
      return range_from_cstr("Expected a type definition.");
    }
    case FailCode::expected_semicolon: {
      return range_from_cstr("Expected a semicolon.");
    }
    case FailCode::expected_colon_or_semicolon: {
      return range_from_cstr("Expected ':' or ';'.");
    }
    case FailCode::expected_colon_after_type_name: {
      return range_from_cstr("Expected ':' after a type name.");
    }
    case FailCode::expected_colon_after_field_name: {
      return range_from_cstr("Expected ':' after a field name.");
    }
    case FailCode::expected_opening_curly_bracket: {
      return range_from_cstr("Expected '{'.");
    }
    case FailCode::expected_closing_square_bracket: {
      return range_from_cstr("Expected ']'.");
    }
    case FailCode::keyword_reserved: {
      return range_from_cstr("Can't use a keyword as a name.");
    }
    case FailCode::ok: {
      return range_from_cstr("No error to descibe, please report this.");
    }
    case FailCode::backtrack: {
      return range_from_cstr("Internal backtracking error, please report this.");
    }
    default: {
      return range_from_cstr("Unknown error, please report this.");
    }
  }
}

// Parser state. Used everywhere as part of the context.
// Can be copied and restored to backtrack.
struct ParserState {
  // The cursor is the index of the next byte to be parsed.
  U64 cursor;

  // The top level definitions are stored here.
  // Since we only ever append definitions, and `CrudeDynamicArray` preserves
  // the contents of the old array, we can safely restore the state.
  CrudeDynamicArray<TopLevelDefinition> top_level_definitions;

  // On a failure, this is set. The cursor can be used to identify the
  // location of the failure.
  Fail fail;
};

// Parser context. Used in all parsing functions.
// State is considered mutable; everything else is not.
struct ParserContext {
  // Arena for all allocations during parsing.
  vm::LinearArena *arena;

  // The input to be parsed, as an ASCII string.
  Range<U8> input;

  // The current state of the parser.
  ParserState state;
};

using Ctx = ParserContext *;

void parse_struct(Ctx ctx, Range<U8> definition_name, StructDefinition *result);
void parse_choice(Ctx ctx, Range<U8> definition_name, ChoiceDefinition *result);

// Helps to set the fail flag and cursor.
static inline
void set_fail(Ctx ctx, FailCode code) {
  if (ctx->state.fail.code == FailCode::ok) {
    ctx->state.fail = {
      .code = code,
      .cursor = ctx->state.cursor,
    };
  }
}

// Peek one byte (= one character), if possible.
static inline
U8 peek_byte(Ctx ctx) {
  if (ctx->state.cursor == ctx->input.count) {
    // Since '\0' is not valid grammar, we use it to signal failure.
    return 0;
  }
  return ctx->input.pointer[ctx->state.cursor];
}

// Skip whitespace. When there is no whitespace, do nothing.
void skip_whitespace(Ctx ctx, bool allow_newlines = true) {
  while (true) {
    auto byte = peek_byte(ctx);

    if (0
      || byte == ' '
      || byte == '\t'
      || byte == '\r'
      || (byte == '\n' && allow_newlines)
    ) {
      ctx->state.cursor++;
    } else {
      // Check for a comment. Do we have "//"?
      Bool is_comment = false;
      if (ctx->state.cursor + 1 < ctx->input.count) {
        auto next_byte = ctx->input.pointer[ctx->state.cursor + 1];
        if (byte == '/' && next_byte == '/') {
          is_comment = true;
          ctx->state.cursor += 2;
          // Skip until the end of the line.
          while (true) {
            auto byte = peek_byte(ctx);

            if (byte == '\0') {
              return;
            } else if (byte == '\n') {
              if (allow_newlines) {
                ctx->state.cursor++;
                break;
              } else {
                return;
              }
            }

            ctx->state.cursor++;
          }
        }
      }

      if (!is_comment) {
        break;
      }
    }
  }
}

// Used after #directives.
void skip_whitespace_and_exactly_one_newline(Ctx ctx) {
  // First, skip all whitespace at the end of the line.
  skip_whitespace(ctx, false);

  // Now, the next character must be a newline.
  {
    auto byte = peek_byte(ctx);

    if (byte == '\n') {
      ctx->state.cursor++;
    } else {
      set_fail(ctx, FailCode::expected_newline);
    }
  }
}

// Name kinds (type, value, schema) have different conventions.
enum class NameKind {
  lowercase,
  uppercase,
  type = uppercase,
  value = lowercase,
  schema = uppercase,
};

// Parse a name. The name kind determines the allowed characters.
// Returns an empty range on failure.
Range<U8> parse_name(Ctx ctx, NameKind kind) {
  UInt cursor_start = ctx->state.cursor;

  // Check the first character.
  {
    auto byte = peek_byte(ctx);

    if (0
      || (kind == NameKind::type && byte >= 'A' && byte <= 'Z')
      || (kind == NameKind::value && byte >= 'a' && byte <= 'z')
    ) {
      ctx->state.cursor++;
    } else {
      set_fail(
        ctx, kind == NameKind::uppercase
          ? FailCode::expected_uppercase_letter
          : FailCode::expected_lowercase_letter
      );
      return {};
    }
  }

  // Now, consume the rest of the name.
  while (true) {
    auto byte = peek_byte(ctx);

    if (0
      || (byte >= 'A' && byte <= 'Z')
      || (byte >= 'a' && byte <= 'z')
      || (byte >= '0' && byte <= '9')
    ) {
      ctx->state.cursor++;
    } else {
      break;
    }
  }

  auto result = Range<U8> {
    .pointer = (ctx->input.pointer + cursor_start),
    .count = ctx->state.cursor - cursor_start,
  };

  if (
    range_equal(result, range_from_cstr("struct")) ||
    range_equal(result, range_from_cstr("choice"))
  ) {
    ctx->state.cursor = cursor_start;
    set_fail(ctx, FailCode::keyword_reserved);
    return {};
  }

  return result;
}

Range<U8> parse_type_name(Ctx ctx) { return parse_name(ctx, NameKind::type); }
Range<U8> parse_value_name(Ctx ctx) { return parse_name(ctx, NameKind::value); }

// Skip exactly the given character, otherwise fail.
void skip_specific_character(Ctx ctx, U8 ascii_character, FailCode fail_code) {
  auto byte = peek_byte(ctx);

  if (byte == ascii_character) {
    ctx->state.cursor++;
  } else {
    set_fail(ctx, fail_code);
  }
}

// Skip exactly the given string, otherwise fail.
void skip_specific_cstring(Ctx ctx, char const* cstr, FailCode fail_code) {
  auto clen = strlen(cstr);
  if (ctx->state.cursor + clen > ctx->input.count) {
    set_fail(ctx, fail_code);
    return;
  }

  for (UInt i = 0; i < clen; i++) {
    if (ctx->input.pointer[ctx->state.cursor + i] != cstr[i]) {
      ctx->state.cursor += i;
      set_fail(ctx, fail_code);
      return;
    }
  }

  ctx->state.cursor += clen;
}

// Parse a reference to a type.
Type parse_type_reference(Ctx ctx) {
  auto name = parse_type_name(ctx);
  auto name_hash = hash64::from_name(name);

  // In case none of the below matches, we assume it is a user-defined type.
  // It will be unresolved at this point.
  auto concrete_type = ConcreteType {
    .which = ConcreteType::Which::defined,
    .defined = {
      .top_level_definition_name_hash = name_hash,
    },
  };

  if (name.count > 0) {
    if (range_equal(name, range_from_cstr("U8"))) {
      concrete_type = { .which = ConcreteType::Which::u8 };
    } else if (range_equal(name, range_from_cstr("U16"))) {
      concrete_type = { .which = ConcreteType::Which::u16 };
    } else if (range_equal(name, range_from_cstr("U32"))) {
      concrete_type = { .which = ConcreteType::Which::u32 };
    } else if (range_equal(name, range_from_cstr("U64"))) {
      concrete_type = { .which = ConcreteType::Which::u64 };
    } else if (range_equal(name, range_from_cstr("I8"))) {
      concrete_type = { .which = ConcreteType::Which::i8 };
    } else if (range_equal(name, range_from_cstr("I16"))) {
      concrete_type = { .which = ConcreteType::Which::i16 };
    } else if (range_equal(name, range_from_cstr("I32"))) {
      concrete_type = { .which = ConcreteType::Which::i32 };
    } else if (range_equal(name, range_from_cstr("I64"))) {
      concrete_type = { .which = ConcreteType::Which::i64 };
    } else if (range_equal(name, range_from_cstr("F32"))) {
      concrete_type = { .which = ConcreteType::Which::f32 };
    } else if (range_equal(name, range_from_cstr("F64"))) {
      concrete_type = { .which = ConcreteType::Which::f64 };
    }
  }

  // Now, check for sequences and references.
  skip_whitespace(ctx);
  auto byte = peek_byte(ctx);

  if (byte == '*') {
    ctx->state.cursor++;
    return {
      .which = Type::Which::reference,
      .reference = {
        .type = concrete_type,
      },
    };
  } else if (byte == '[') {
    ctx->state.cursor++;
    skip_whitespace(ctx);
    skip_specific_character(ctx, ']', FailCode::expected_closing_square_bracket);
    return {
      .which = Type::Which::sequence,
      .sequence = {
        .element_type = concrete_type,
      },
    };
  } else {
    return {
      .which = Type::Which::concrete,
      .concrete = {
        .type = concrete_type,
      },
    };
  }
}

// Parse a type definition, which currently may be either a struct or a choice.
U64 parse_type_definition(Ctx ctx, Range<U8> definition_name) {
  // Save the state, so we can backtrack.
  auto saved_state = ctx->state;

  skip_specific_cstring(ctx, "struct", FailCode::backtrack);

  if (ctx->state.fail.code == FailCode::ok) {
    // We know it's a struct. Backtrack and parse it.
    TopLevelDefinition result = {};
    parse_struct(ctx, definition_name, &result.a_struct);
    result.which = TopLevelDefinition::Which::a_struct;

    auto defs = &ctx->state.top_level_definitions;
    ensure_one_more(defs, ctx->arena);
    defs->pointer[defs->count] = result;
    defs->count++;

    return result.a_struct.name_hash;
  }

  // If we failed to parse a struct, try parsing a choice.
  // Backtrack to the beginning of the type definition.
  ctx->state = saved_state;
  skip_specific_cstring(ctx, "choice", FailCode::backtrack);

  if (ctx->state.fail.code == FailCode::ok) {
    // We know it's a choice. Backtrack and parse it.
    TopLevelDefinition result = {};
    parse_choice(ctx, definition_name, &result.a_choice);
    result.which = TopLevelDefinition::Which::a_choice;

    auto defs = &ctx->state.top_level_definitions;
    ensure_one_more(defs, ctx->arena);
    defs->pointer[defs->count] = result;
    defs->count++;

    return result.a_choice.name_hash;
  }

  // It is neither, so we should backtrack and fail.
  ctx->state = saved_state;
  set_fail(ctx, FailCode::expected_type_definition);
  return U64(-1);
}

// Can return non-sensical types, callers need to check for parsing failure.
Type parse_type_reference_or_inline_type_definition(
  Ctx ctx,
  Range<U8> parent_name,
  Range<U8> part_name
) {
  auto saved_state = ctx->state;

  parse_name(ctx, NameKind::type);

  if (ctx->state.fail.code == FailCode::ok) {
    // It's must be a type reference.
    ctx->state = saved_state;
    auto type = parse_type_reference(ctx);
    return type;
  }

  // It wasn't a type, so it must be an inline type definition.
  ctx->state = saved_state;

  // We rely on `vm` allocating contiguous memory.
  auto definition_name = Range<U8> {
    .pointer = (U8 *) vm::realign(ctx->arena),
    .count = 0,
  };

  // Concatenate the parent name, separator, and the part name.
  {
    auto dst = vm::many<U8>(ctx->arena, parent_name.count);
    range_copy(dst, parent_name);
    definition_name.count += parent_name.count;
  }
  {
    auto str = range_from_cstr("_");
    auto dst = vm::many<U8>(ctx->arena, str.count);
    range_copy(dst, str);
    definition_name.count += str.count;
  }
  {
    auto dst = vm::many<U8>(ctx->arena, part_name.count);
    range_copy(dst, part_name);
    dst.pointer[0] = toupper(dst.pointer[0]); // Capitalize the first letter.
    definition_name.count += part_name.count;
  }

  auto hash = parse_type_definition(ctx, definition_name);

  if (ctx->state.fail.code == FailCode::ok) {
    return {
      .which = Type::Which::concrete,
      .concrete = {
        .type = {
          .which = ConcreteType::Which::defined,
          .defined = {
            .top_level_definition_name_hash = hash,
          },
        },
      },
    };
  }

  // It wasn't a type definition either, so we fail.
  return {};
}

// Parse a choice, beginning from the opening curly bracket.
void parse_choice(Ctx ctx, Range<U8> definition_name, ChoiceDefinition *result) {
  skip_whitespace(ctx);
  skip_specific_character(ctx, '{', FailCode::expected_opening_curly_bracket);
  skip_whitespace(ctx);

  // Accumulate options here.
  CrudeDynamicArray<OptionDefinition> dynamic_options = {};

  while (true)
  {
    auto saved_state = ctx->state;

    skip_specific_character(ctx, '}', FailCode::backtrack);
    skip_whitespace(ctx);

    if (ctx->state.fail.code == FailCode::ok) {
      // Finished.
      break;
    }

    ctx->state = saved_state;
    Type type = {};
    auto option_name = parse_value_name(ctx);

    if (ctx->state.fail.code == FailCode::ok) {
      // Save post-name state, because the zero-sized option syntax forces us to
      // possibly backtrack.
      auto post_name_state = ctx->state;

      // Optimistically parse as zero-sized option.
      skip_whitespace(ctx);
      skip_specific_character(ctx, ';', FailCode::backtrack);
      skip_whitespace(ctx);

      if (ctx->state.fail.code == FailCode::ok) {
        type = {
          .which = Type::Which::concrete,
          .concrete = {
            .type = {
              .which = ConcreteType::Which::zero_sized,
            },
          },
        };
      } else {
        // If we could not, assume it's a normal option.
        ctx->state = post_name_state;
        skip_specific_character(ctx, ':', FailCode::backtrack);

        if (ctx->state.fail.code == FailCode::ok) {
          skip_whitespace(ctx);
          type = parse_type_reference_or_inline_type_definition(ctx, definition_name, option_name);

          skip_whitespace(ctx);
          skip_specific_character(ctx, ';', FailCode::expected_semicolon);
          skip_whitespace(ctx);
        } else {
          ctx->state = post_name_state;
          set_fail(ctx, FailCode::expected_colon_or_semicolon);
        }
      }
    }

    if (ctx->state.fail.code != FailCode::ok) {
      return;
    }

    // Success. Add the option to the array.
    ensure_one_more(&dynamic_options, ctx->arena);
    dynamic_options.pointer[dynamic_options.count++] = OptionDefinition {
      .name = option_name,
      .name_hash = hash64::from_name(option_name),
      .type = type,
    };
  }

  result->name = definition_name;
  result->name_hash = hash64::from_name(definition_name);
  result->options = {
    .pointer = dynamic_options.pointer,
    .count = dynamic_options.count,
  };
}

// Parse a struct, beginning from the opening curly bracket.
void parse_struct(Ctx ctx, Range<U8> definition_name, StructDefinition *result) {
  skip_whitespace(ctx);
  skip_specific_character(ctx, '{', FailCode::expected_opening_curly_bracket);
  skip_whitespace(ctx);

  // Accumulate fields here.
  CrudeDynamicArray<FieldDefinition> dynamic_fields = {};

  while (true) {
    // This inner loop is similar to the one in `parse_choice`, but
    // it is simpler because we don't have to deal with zero-sized fields here.
    auto saved_state = ctx->state;

    skip_specific_character(ctx, '}', FailCode::backtrack);
    skip_whitespace(ctx);

    if (ctx->state.fail.code == FailCode::ok) {
      // Finished.
      break;
    }

    ctx->state = saved_state;
    auto field_name = parse_value_name(ctx);

    skip_specific_character(ctx, ':', FailCode::expected_colon_after_field_name);
    skip_whitespace(ctx);
    auto type = parse_type_reference_or_inline_type_definition(ctx, definition_name, field_name);

    skip_whitespace(ctx);
    skip_specific_character(ctx, ';', FailCode::expected_semicolon);
    skip_whitespace(ctx);

    if (ctx->state.fail.code != FailCode::ok) {
      return;
    }

    // Success. Add the field to the array.
    ensure_one_more(&dynamic_fields, ctx->arena);
    dynamic_fields.pointer[dynamic_fields.count++] = FieldDefinition {
      .name = field_name,
      .name_hash = hash64::from_name(field_name),
      .type = type,
    };
  }

  result->name = definition_name;
  result->name_hash = hash64::from_name(definition_name);
  result->fields = {
    .pointer = dynamic_fields.pointer,
    .count = dynamic_fields.count,
  };

}

// Currently, the only top-level definitions are type definitions.
// So, this is straightforward.
void parse_top_level_definition(Ctx ctx) {
  auto definition_name = parse_type_name(ctx);
  skip_specific_character(ctx, ':', FailCode::expected_colon_after_type_name);
  skip_whitespace(ctx);
  parse_type_definition(ctx, definition_name);
  skip_whitespace(ctx);
  skip_specific_character(ctx, ';', FailCode::expected_semicolon);
  skip_whitespace(ctx);
}

// Parse a #name directive.
Range<U8> parse_directive_name(Ctx ctx) {
  skip_specific_cstring(ctx, "#name", FailCode::expected_name_directive);
  skip_whitespace(ctx);
  auto result = parse_name(ctx, NameKind::schema);
  skip_whitespace_and_exactly_one_newline(ctx);
  return result;
}

// Parse the whole schema.
ParseResult parse_input(vm::LinearArena *arena, Range<U8> input) {
  ParserContext ctx_ = {
    .arena = arena,
    .input = input,
    .state = {},
  };
  auto ctx = &ctx_;
  auto root = vm::one<Root>(ctx->arena);

  // #name directive must come first.
  auto schema_name = parse_directive_name(ctx);
  skip_whitespace(ctx);

  while (true) {
    // Stop parsing if we have encountered an error.
    if (ctx->state.fail.code != FailCode::ok) {
      return {
        .fail = ctx->state.fail,
      };
    }

    // Check for EOF.
    if (ctx->state.cursor == ctx->input.count) {
      break;
    }

    // Currently, the schema can only contain top-level definitions.
    parse_top_level_definition(ctx);
    skip_whitespace(ctx);
  }

  *root = {
    .schema_name = schema_name,
    .schema_name_hash = hash64::from_name(schema_name),
    .definitions = {
      .pointer = ctx->state.top_level_definitions.pointer,
      .count = ctx->state.top_level_definitions.count,
    },
  };

  return {
    .root = root,
  };
}

} // namespace parsing
