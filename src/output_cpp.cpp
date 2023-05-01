#include <cstring>

#include "platform.hpp"
#include "grammar.hpp"

namespace output_cpp {

struct OutputContext {
  vm::LinearArena *dedicated_arena;
  Range<Byte> range;
  grammar::Root *root;
};

using Ctx = OutputContext *;

void output_cstring(Ctx ctx, const char *cstr) {
  auto clen = strlen(cstr);
  auto range = vm::allocate_many<Byte>(ctx->dedicated_arena, clen);
  ASSERT(range.pointer);
  memcpy(range.pointer, cstr, clen);
  ctx->range.count += clen;
}

void output_range(Ctx ctx, Range<U8> range) {
  auto new_range = vm::allocate_many<Byte>(ctx->dedicated_arena, range.count);
  ASSERT(new_range.pointer);
  memcpy(new_range.pointer, range.pointer, range.count);
  ctx->range.count += range.count;
}

void output_struct_declaration(Ctx ctx, grammar::StructDefinition *it) {
  output_cstring(ctx, "struct ");
  output_range(ctx, it->name);
  output_cstring(ctx, ";\n");
}

void output_choice_declaration(Ctx ctx, grammar::ChoiceDefinition *it) {
  output_cstring(ctx, "enum class ");
  output_range(ctx, it->name);
  output_cstring(ctx, "_enum;\n");

  output_cstring(ctx, "union ");
  output_range(ctx, it->name);
  output_cstring(ctx, "_union;\n");
}

void output_concrete_type_name(Ctx ctx, grammar::ConcreteType *concrete_type) {
  switch (concrete_type->which) {
    case grammar::ConcreteType::Which::defined: {
      auto definition = ctx->root->definitions.pointer + concrete_type->defined.top_level_definition_index;
      switch (definition->which) {
        case grammar::TopLevelDefinition::Which::a_struct: {
          output_range(ctx, definition->a_struct.name);
          break;
        }
        case grammar::TopLevelDefinition::Which::a_choice: {
          output_range(ctx, definition->a_choice.name);
          break;
        }
        default: {
          ASSERT(false);
        }
      }
      break;
    }
    case grammar::ConcreteType::Which::u8: {
      output_cstring(ctx, "U8");
      break;
    }
    case grammar::ConcreteType::Which::u16: {
      output_cstring(ctx, "U16");
      break;
    }
    case grammar::ConcreteType::Which::u32: {
      output_cstring(ctx, "U32");
      break;
    }
    case grammar::ConcreteType::Which::u64: {
      output_cstring(ctx, "U64");
      break;
    }
    case grammar::ConcreteType::Which::i8: {
      output_cstring(ctx, "I8");
      break;
    }
    case grammar::ConcreteType::Which::i16: {
      output_cstring(ctx, "I16");
      break;
    }
    case grammar::ConcreteType::Which::i32: {
      output_cstring(ctx, "I32");
      break;
    }
    case grammar::ConcreteType::Which::i64: {
      output_cstring(ctx, "I64");
      break;
    }
    case grammar::ConcreteType::Which::f32: {
      output_cstring(ctx, "F32");
      break;
    }
    case grammar::ConcreteType::Which::f64: {
      output_cstring(ctx, "F64");
      break;
    }
    default: {
      ASSERT(false);
    }
  }
}

void output_type(Ctx ctx, grammar::Type *it) {
  switch (it->which) {
    case grammar::Type::Which::concrete: {
      auto concrete_type = &it->concrete.type;
      output_concrete_type_name(ctx, concrete_type);
      break;
    }
    case grammar::Type::Which::pointer: {
      output_concrete_type_name(ctx, &it->pointer.type);
      output_cstring(ctx, "*");
      break;
    }
    case grammar::Type::Which::flexible_array: {
      output_cstring(ctx, "Array<");
      output_concrete_type_name(ctx, &it->flexible_array.element_type);
      output_cstring(ctx, ">");
      break;
    }
    default: {
      ASSERT(false);
    }
  }
}

enum class TypePlurality {
  zero,
  one,
  enum_and_union,
};

TypePlurality get_plurality(Ctx ctx, grammar::Type *it) {
  if (it->which == grammar::Type::Which::concrete) {
    if (it->concrete.type.which == grammar::ConcreteType::Which::zero_sized) {
      return TypePlurality::zero;
    }
    // TODO: Check for zero-sized arrays?

    if (it->concrete.type.which == grammar::ConcreteType::Which::defined) {
      auto definition = it->concrete.type.defined.top_level_definition_index;
      auto top_level_definition = ctx->root->definitions.pointer + definition;
      switch (top_level_definition->which) {
        case grammar::TopLevelDefinition::Which::a_struct: {
          return TypePlurality::one;
        }
        case grammar::TopLevelDefinition::Which::a_choice: {
          return TypePlurality::enum_and_union;
        }
        default: {
          ASSERT(false);
        }
      }
    }
  }
  return TypePlurality::one;
}

void output_struct(Ctx ctx, grammar::StructDefinition *it) {
  output_cstring(ctx, "struct ");
  output_range(ctx, it->name);
  output_cstring(ctx, " {\n");
  for (size_t i = 0; i < it->fields.count; i++) {
    auto field = it->fields.pointer + i;
    auto plurality = get_plurality(ctx, &field->type);
    switch (plurality) {
      case TypePlurality::zero: {
        continue;
      }
      case TypePlurality::one: {
        output_cstring(ctx, "  ");
        output_type(ctx, &field->type);
        output_cstring(ctx, " ");
        output_range(ctx, field->name);
        output_cstring(ctx, ";\n");
        break;
      }
      case TypePlurality::enum_and_union: {
        ASSERT(field->type.which == grammar::Type::Which::concrete);
        output_cstring(ctx, "  ");
        output_concrete_type_name(ctx, &field->type.concrete.type);
        output_cstring(ctx, "_enum ");
        output_range(ctx, field->name);
        output_cstring(ctx, "_enum;\n");
        output_cstring(ctx, "  ");
        output_concrete_type_name(ctx, &field->type.concrete.type);
        output_cstring(ctx, "_union ");
        output_range(ctx, field->name);
        output_cstring(ctx, "_union;\n");
        break;
      }
      default: {
        ASSERT(false);
      }
    }
  }
  output_cstring(ctx, "};\n\n");
}

void output_choice(Ctx ctx, grammar::ChoiceDefinition *it) {
  output_cstring(ctx, "enum class ");
  output_range(ctx, it->name);
  output_cstring(ctx, "_enum {\n");
  for (size_t i = 0; i < it->options.count; i++) {
    auto option = it->options.pointer + i;
    output_cstring(ctx, "  ");
    output_range(ctx, option->name);
    output_cstring(ctx, ",\n");
  }
  output_cstring(ctx, "};\n\n");

  output_cstring(ctx, "union ");
  output_range(ctx, it->name);
  output_cstring(ctx, "_union {\n");
  for (size_t i = 0; i < it->options.count; i++) {
    auto option = it->options.pointer + i;
    auto plurality = get_plurality(ctx, &option->type);
    if (plurality == TypePlurality::zero) {
      continue;
    }
    ASSERT(plurality == TypePlurality::one);
    output_cstring(ctx, "  ");
    output_type(ctx, &option->type);
    output_cstring(ctx, " ");
    output_range(ctx, option->name);
    output_cstring(ctx, ";\n");
  }
  output_cstring(ctx, "};\n\n");
}

char const *output_cpp_header = R"(#include <cstdint>

namespace svf {

using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

using I8 = int8_t;
using I16 = int16_t;
using I32 = int32_t;
using I64 = int64_t;

using F32 = float;
using F64 = double;

template<typename T>
struct Array {
  uint32_t pointer_offset;
  uint32_t count;
};
  
namespace schema_format_0 {
#pragma pack(push, 1)
)";

char const *output_cpp_footer = R"(#pragma pack(pop)
} // namespace schema_format_0

} // namespace svf
)";

Range<Byte> output_code(
  grammar::Root *root,
  Range<grammar::OrderElement> ordering,
  vm::LinearArena *arena,
  vm::LinearArena *output_arena
) {
  // We rely on `vm` allocating contiguous memory.
  auto range = vm::allocate_many<Byte>(output_arena, 0);
  OutputContext context_value = {
    .dedicated_arena = output_arena,
    .range = range,
    .root = root,
  };
  auto ctx = &context_value;

  output_cstring(ctx, output_cpp_header);

  output_cstring(ctx, "\n// Forward declarations\n");
  for (size_t i = 0; i < root->definitions.count; i++) {
    auto definition = root->definitions.pointer + ordering.pointer[i].index;
    switch (definition->which) {
      case grammar::TopLevelDefinition::Which::a_struct: {
        output_struct_declaration(ctx, &definition->a_struct);
        break;
      }
      case grammar::TopLevelDefinition::Which::a_choice: {
        output_choice_declaration(ctx, &definition->a_choice);
        break;
      }
      default: {
        ASSERT(false);
      }
    }
  }

  output_cstring(ctx, "\n// Full declarations\n");
  for (size_t i = 0; i < root->definitions.count; i++) {
    auto definition = root->definitions.pointer + ordering.pointer[i].index;
    switch (definition->which) {
      case grammar::TopLevelDefinition::Which::a_struct: {
        output_struct(ctx, &definition->a_struct);
        break;
      }
      case grammar::TopLevelDefinition::Which::a_choice: {
        output_choice(ctx, &definition->a_choice);
        break;
      }
      default: {
        ASSERT(false);
      }
    }
  }

  output_cstring(ctx, output_cpp_footer);

  return ctx->range;
}

} // namespace output_cpp
