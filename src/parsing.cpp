#include <cstring>

#include "platform.hpp"
#include "grammar.hpp"

namespace parsing {

using namespace grammar;

struct ParserState {
  U64 cursor;
  DynamicArray<TopLevelDefinition> top_level_definitions;
  struct Fail {
    Bool flag;
    U64 cursor;
  } fail;
};

struct ParserContext {
  vm::LinearArena *arena;
  Range<U8> input;
  ParserState state;
};

using Ctx = ParserContext *;

void parse_struct(Ctx ctx, Range<Byte> definition_name, StructDefinition *result);
void parse_choice(Ctx ctx, Range<Byte> definition_name, ChoiceDefinition *result);

static inline
U64 compute_name_hash(Range<Byte> name) {
  auto hash = hash64::begin();
  hash64::add_bytes(&hash, name);
  return hash;
}

static inline
void set_fail(Ctx ctx) {
  if (!ctx->state.fail.flag) {
    ctx->state.fail = {
      .flag = true,
      .cursor = ctx->state.cursor,
    };
  }
}

void skip_whitespace(Ctx ctx) {
  while (true) {
    if (ctx->state.cursor == ctx->input.count) {
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
  if (ctx->state.cursor == ctx->input.count) {
    set_fail(ctx);
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
      set_fail(ctx);
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
  if (ctx->state.cursor == ctx->input.count) {
    set_fail(ctx);
    return;
  }

  auto byte = ctx->input.pointer[ctx->state.cursor];

  if (byte == ascii_character) {
    ctx->state.cursor++;
  } else {
    set_fail(ctx);
  }
}

void skip_specific_cstring(Ctx ctx, char const* cstr) {
  auto clen = strlen(cstr);
  if (ctx->state.cursor + clen > ctx->input.count) {
    set_fail(ctx);
    return;
  }

  for (size_t i = 0; i < clen; i++) {
    if (ctx->input.pointer[ctx->state.cursor + i] != cstr[i]) {
      ctx->state.cursor += i;
      set_fail(ctx);
    }
  }

  ctx->state.cursor += clen;
}

Type parse_type(Ctx ctx) {
  auto name = parse_type_name(ctx);
  skip_whitespace(ctx);

  if (ctx->state.cursor == ctx->input.count) {
    set_fail(ctx);
    return {};
  }

  auto byte = ctx->input.pointer[ctx->state.cursor];
  auto name_hash = compute_name_hash(name);

  auto concrete_type = ConcreteType {
    .which = ConcreteType::Which::unresolved,
    .unresolved = {
      .name = name,
      .name_hash = name_hash,
    },
  };

  static_assert(sizeof(char) == 1); // sanity check
  if (strncmp("U8", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::u8 };
  } else if (strncmp("U16", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::u16 };
  } else if (strncmp("U32", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::u32 };
  } else if (strncmp("U64", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::u64 };
  } else if (strncmp("I8", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::i8 };
  } else if (strncmp("I16", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::i16 };
  } else if (strncmp("I32", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::i32 };
  } else if (strncmp("I64", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::i64 };
  } else if (strncmp("F32", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::f32 };
  } else if (strncmp("F64", (char const *) name.pointer, name.count) == 0) {
    concrete_type = { .which = ConcreteType::Which::f64 };
  }

  if (byte == '*') {
    return {
      .which = Type::Which::pointer,
      .pointer = {
        .type = concrete_type,
      },
    };
  } else if (byte == '[') {
    ctx->state.cursor++;
    skip_whitespace(ctx);
    skip_specific_character(ctx, ']');
    return {
      .which = Type::Which::flexible_array,
      .flexible_array = {
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

U32 parse_type_definition(Ctx ctx, Range<Byte> definition_name) {
  auto saved_state = ctx->state;
  skip_specific_cstring(ctx, "struct");
  TopLevelDefinition result;

  if (!ctx->state.fail.flag) {
    ctx->state = saved_state;
    parse_struct(ctx, definition_name, &result.a_struct);
    result.which = TopLevelDefinition::Which::a_struct;

    dynamic_resize(&ctx->state.top_level_definitions, ctx->arena, ctx->state.top_level_definitions.count + 1);
    ASSERT(ctx->state.top_level_definitions.pointer); // temporary?
    ctx->state.top_level_definitions.pointer[ctx->state.top_level_definitions.count] = result;
    return ctx->state.top_level_definitions.count++;
  }

  ctx->state = saved_state;
  skip_specific_cstring(ctx, "choice");

  if (!ctx->state.fail.flag) {
    ctx->state = saved_state;
    parse_choice(ctx, definition_name, &result.a_choice);
    result.which = TopLevelDefinition::Which::a_choice;

    dynamic_resize(&ctx->state.top_level_definitions, ctx->arena, ctx->state.top_level_definitions.count + 1);
    ASSERT(ctx->state.top_level_definitions.pointer); // temporary?
    ctx->state.top_level_definitions.pointer[ctx->state.top_level_definitions.count] = result;
    return ctx->state.top_level_definitions.count++;
  }

  ctx->state = saved_state;
  set_fail(ctx);
  return U32(-1);
}

Type parse_type_or_inline_type_definition(Ctx ctx, Range<Byte> parent_name, Range<Byte> part_name) {
  auto saved_state = ctx->state;
  auto type = parse_type(ctx);

  if (!ctx->state.fail.flag) {
    return type;
  }

  // We rely on `vm` allocating contiguous memory.
  auto definition_name = vm::allocate_many<Byte>(ctx->arena, 0);
  {
    auto ptr = vm::allocate_many<Byte>(ctx->arena, parent_name.count).pointer;
    ASSERT(ptr); // temporary?
    memcpy(ptr, parent_name.pointer, parent_name.count);
    definition_name.count += parent_name.count;
  }
  {
    char const cstr[] = "_dot_";
    size_t clen = strlen(cstr);
    auto ptr = vm::allocate_many<Byte>(ctx->arena, clen).pointer;
    ASSERT(ptr); // temporary?
    memcpy(ptr, cstr, clen);
    definition_name.count += clen;
  }
  {
    auto ptr = vm::allocate_many<Byte>(ctx->arena, part_name.count).pointer;
    ASSERT(ptr); // temporary?
    memcpy(ptr, part_name.pointer, part_name.count);
    definition_name.count += part_name.count;
  }

  // It wasn't a type, so it must be an inline type definition.
  ctx->state = saved_state;
  auto index = parse_type_definition(ctx, definition_name);

  if (!ctx->state.fail.flag) {
    return {
      .which = Type::Which::concrete,
      .concrete = {
        .type = {
          .which = ConcreteType::Which::defined,
          .defined = {
            .top_level_definition_index = index,
          },
        },
      },
    };
  }

  return {};
}

void parse_choice(Ctx ctx, Range<Byte> definition_name, ChoiceDefinition *result) {
  skip_specific_cstring(ctx, "choice");
  skip_whitespace(ctx);
  skip_specific_character(ctx, '{');
  skip_whitespace(ctx);

  DynamicArray<OptionDefinition> dynamic_options = {};
  ParserState::Fail option_fail = {};

  while (true)
  {
    auto saved_state = ctx->state;
    auto option_name = parse_value_name(ctx);

    Type type;
    if (!ctx->state.fail.flag) {
      auto post_name_state = ctx->state;
      skip_specific_character(ctx, ':');
      skip_whitespace(ctx);
      type = parse_type_or_inline_type_definition(ctx, definition_name, option_name);

      if (ctx->state.fail.flag) {
        ctx->state = post_name_state;
        type = {
          .which = Type::Which::concrete,
          .concrete = {
            .type = {
              .which = ConcreteType::Which::zero_sized,
            },
          },
        };
      }

      skip_whitespace(ctx);
      skip_specific_character(ctx, ';');
      skip_whitespace(ctx);
    }

    if (ctx->state.fail.flag) {
      option_fail = ctx->state.fail;
      ctx->state = saved_state;
      break;
    }

    dynamic_resize(&dynamic_options, ctx->arena, dynamic_options.count + 1);
    ASSERT(dynamic_options.pointer); // temporary?
    dynamic_options.pointer[dynamic_options.count++] = OptionDefinition {
      .name = option_name,
      .name_hash = compute_name_hash(option_name),
      .type = type,
    };
  }

  result->name = definition_name;
  result->name_hash = compute_name_hash(definition_name);
  result->options = {
    .pointer = dynamic_options.pointer,
    .count = dynamic_options.count,
  };

  skip_whitespace(ctx);
  skip_specific_character(ctx, '}');

  if (option_fail.flag && ctx->state.fail.flag) {
    ctx->state.fail = option_fail;
  }
}

void parse_struct(Ctx ctx, Range<Byte> definition_name, StructDefinition *result) {
  skip_specific_cstring(ctx, "struct");
  skip_whitespace(ctx);
  skip_specific_character(ctx, '{');
  skip_whitespace(ctx);

  DynamicArray<FieldDefinition> dynamic_fields = {};
  ParserState::Fail field_fail = {};

  while (true) {
    auto saved_state = ctx->state;
    auto field_name = parse_value_name(ctx);

    skip_specific_character(ctx, ':');
    skip_whitespace(ctx);

    if (ctx->state.fail.flag) {
      field_fail = ctx->state.fail;
      ctx->state = saved_state;
      break;
    }

    auto type = parse_type_or_inline_type_definition(ctx, definition_name, field_name);

    skip_whitespace(ctx);
    skip_specific_character(ctx, ';');
    skip_whitespace(ctx);

    if (ctx->state.fail.flag) {
      field_fail = ctx->state.fail;
      ctx->state = saved_state;
      break;
    }

    dynamic_resize(&dynamic_fields, ctx->arena, dynamic_fields.count + 1);
    ASSERT(dynamic_fields.pointer); // temporary?
    dynamic_fields.pointer[dynamic_fields.count++] = FieldDefinition {
      .name = field_name,
      .name_hash = compute_name_hash(field_name),
      .type = type,
    };
  }

  result->name = definition_name;
  result->name_hash = compute_name_hash(definition_name);
  result->fields = {
    .pointer = dynamic_fields.pointer,
    .count = dynamic_fields.count,
  };

  skip_whitespace(ctx);
  skip_specific_character(ctx, '}');

  if (field_fail.flag && ctx->state.fail.flag) {
    ctx->state.fail = field_fail;
  }
}

void parse_top_level_definition(Ctx ctx) {
  auto definition_name = parse_type_name(ctx);
  skip_specific_character(ctx, ':');
  skip_whitespace(ctx);
  if (ctx->state.fail.flag) {
    return;
  }

  parse_type_definition(ctx, definition_name);

  skip_whitespace(ctx);
  skip_specific_character(ctx, ';');
  skip_whitespace(ctx);
}

Root *parse_input(vm::LinearArena *arena, Range<U8> input) {
  ParserContext context_value = {
    .arena = arena,
    .input = input,
    .state = {},
  };
  auto ctx = &context_value;
  auto root = vm::allocate_one<Root>(ctx->arena);
  ASSERT(root); // temporary?

  skip_whitespace(ctx);
  while (true) {
    if (ctx->state.cursor == ctx->input.count) {
      break;
    }
    parse_top_level_definition(ctx);
    skip_whitespace(ctx);
    if (ctx->state.fail.flag) {
      return 0;
    }
  }

  root->definitions = {
    .pointer = ctx->state.top_level_definitions.pointer,
    .count = ctx->state.top_level_definitions.count,
  };
  return root;
}

} // namespace parsing
