// UNREVIEWED.
#include "../platform.hpp"
#include "../../meta/schema.hpp"
#include "../utilities.hpp"
#include "grammar.hpp"

namespace svf::output_binary {

namespace meta = svf::svf_meta;

void output_concrete_type(
  grammar::ConcreteType* in_concrete,
  meta::ConcreteType_enum* out_enum,
  meta::ConcreteType_union* out_union
) {
  switch (in_concrete->which) {
    case grammar::ConcreteType::Which::u8: {
      *out_enum = meta::ConcreteType_enum::u8;
      break;
    }
    case grammar::ConcreteType::Which::u16: {
      *out_enum = meta::ConcreteType_enum::u16;
      break;
    }
    case grammar::ConcreteType::Which::u32: {
      *out_enum = meta::ConcreteType_enum::u32;
      break;
    }
    case grammar::ConcreteType::Which::u64: {
      *out_enum = meta::ConcreteType_enum::u64;
      break;
    }
    case grammar::ConcreteType::Which::i8: {
      *out_enum = meta::ConcreteType_enum::i8;
      break;
    }
    case grammar::ConcreteType::Which::i16: {
      *out_enum = meta::ConcreteType_enum::i16;
      break;
    }
    case grammar::ConcreteType::Which::i32: {
      *out_enum = meta::ConcreteType_enum::i32;
      break;
    }
    case grammar::ConcreteType::Which::i64: {
      *out_enum = meta::ConcreteType_enum::i64;
      break;
    }
    case grammar::ConcreteType::Which::f32: {
      *out_enum = meta::ConcreteType_enum::f32;
      break;
    }
    case grammar::ConcreteType::Which::f64: {
      *out_enum = meta::ConcreteType_enum::f64;
      break;
    }
    case grammar::ConcreteType::Which::zero_sized: {
      *out_enum = meta::ConcreteType_enum::zero_sized;
      break;
    }
    case grammar::ConcreteType::Which::defined: {
      *out_enum = meta::ConcreteType_enum::defined;
      out_union->defined = {
        .top_level_definition_name_hash = in_concrete->defined.top_level_definition_name_hash,
      };
      break;
    }
    default: {
      ASSERT(false);
    } break;
  }
}

void output_type(
  grammar::Type* in_type,
  meta::Type_enum* out_enum,
  meta::Type_union* out_union
) {
  switch (in_type->which) {
    case grammar::Type::Which::concrete: {
      *out_enum = meta::Type_enum::concrete;
      output_concrete_type(
        &in_type->concrete.type,
        &out_union->concrete.type_enum,
        &out_union->concrete.type_union
      );

      break;
    }
    case grammar::Type::Which::pointer: {
      *out_enum = meta::Type_enum::pointer;
      output_concrete_type(
        &in_type->pointer.type,
        &out_union->pointer.type_enum,
        &out_union->pointer.type_union
      );
      break;
    }
    case grammar::Type::Which::flexible_array: {
      *out_enum = meta::Type_enum::flexible_array;
      output_concrete_type(
        &in_type->flexible_array.element_type,
        &out_union->flexible_array.element_type_enum,
        &out_union->flexible_array.element_type_union
      );
      break;
    }
    default: {
      ASSERT(false);
    } break;
  }
}

Range<Byte> output_bytes(
  grammar::Root *in_root,
  Range<grammar::OrderElement> ordering,
  vm::LinearArena *arena,
  vm::LinearArena *output_arena
) {
  // We rely on `vm` allocating contiguous memory.
  auto range = vm::allocate_many<Byte>(output_arena, 0);

  auto out_schema = vm::allocate_one<meta::Schema>(output_arena);
  ASSERT(out_schema); // temporary

  auto num_structs = 0;
  auto num_choices = 0;
  for (auto i = 0; i < in_root->definitions.count; i++) {
    auto definition = in_root->definitions.pointer + i;
    if (definition->which == grammar::TopLevelDefinition::Which::a_struct) {
      num_structs++;
    } else if (definition->which == grammar::TopLevelDefinition::Which::a_choice) {
      num_choices++;
    } else {
      ASSERT(false);
    }
  }

  auto out_structs = vm::allocate_many<meta::StructDefinition>(
    output_arena,
    num_structs
  );
  ASSERT(out_structs.pointer); // temporary

  auto out_choices = vm::allocate_many<meta::ChoiceDefinition>(
    output_arena,
    num_choices
  );
  ASSERT(out_choices.pointer); // temporary

  U32 struct_index = 0;
  U32 choice_index = 0;

  for (U64 i = 0; i < in_root->definitions.count; i++) {
    auto in_definition = in_root->definitions.pointer + i;
    if (in_definition->which == grammar::TopLevelDefinition::Which::a_struct) {
      auto in_struct = &in_definition->a_struct;
      auto out_struct = out_structs.pointer + struct_index;
      struct_index++;

      auto out_fields = vm::allocate_many<meta::FieldDefinition>(
        output_arena,
        in_struct->fields.count
      );
      ASSERT(out_fields.pointer); // temporary

      *out_struct = {
        .name_hash = in_struct->name_hash,
        .fields = {
          .pointer_offset = offset_between(out_schema, out_fields.pointer),
          .count = safe_int_cast<U32>(out_fields.count),
        },
      };

      for (U64 j = 0; j < in_struct->fields.count; j++) {
        auto in_field = in_struct->fields.pointer + j;
        auto out_field = out_fields.pointer + j;

        *out_field = {
          .name_hash = in_field->name_hash,
        };

        output_type(
          &in_field->type,
          &out_field->type_enum,
          &out_field->type_union
        );
      }
    } else if (in_definition->which == grammar::TopLevelDefinition::Which::a_choice) {
      auto in_choice = &in_definition->a_choice;
      auto out_choice = out_choices.pointer + choice_index;
      choice_index++;

      auto out_options = vm::allocate_many<meta::OptionDefinition>(
        output_arena,
        in_choice->options.count
      );
      ASSERT(out_options.pointer); // temporary

      *out_choice = {
        .name_hash = in_choice->name_hash,
        .options = {
          .pointer_offset = offset_between(out_schema, out_options.pointer),
          .count = safe_int_cast<U32>(out_options.count),
        },
      };

      for (U64 j = 0; j < in_choice->options.count; j++) {
        auto in_option = in_choice->options.pointer + j;
        auto out_option = out_options.pointer + j;

        *out_option = {
          .name_hash = in_option->name_hash,
          .index = in_option->index,
        };

        output_type(
          &in_option->type,
          &out_option->type_enum,
          &out_option->type_union
        );
      }
    } else {
      ASSERT(false);
    }
  }

  *out_schema = {
    .name_hash = in_root->schema_name_hash,
    .structs = {
      .pointer_offset = offset_between(out_schema, out_structs.pointer),
      .count = safe_int_cast<U32>(out_structs.count),
    },
    .choices = {
      .pointer_offset = offset_between(out_schema, out_choices.pointer),
      .count = safe_int_cast<U32>(out_choices.count),
    },
  };

  range.count = output_arena->waterline;
  return range;
}

} // namespace output_binary
