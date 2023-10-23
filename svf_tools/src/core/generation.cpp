#include <src/library.hpp>
#include <src/svf_runtime.h>
#include "../core.hpp"

namespace core::generation {

namespace meta = svf::META;

// TODO @performance: should this be replaced by a hash table?
grammar::TopLevelDefinition *resolve_by_name_hash(
  grammar::Root *in_root,
  U64 name_hash
) {
  for (UInt k = 0; k < in_root->definitions.count; k++) {
    auto definition = in_root->definitions.pointer + k;
    switch (definition->which) {
      case grammar::TopLevelDefinition::Which::a_struct: {
        if (definition->a_struct.name_hash == name_hash) {
          return definition;
        }
        break;
      }
      case grammar::TopLevelDefinition::Which::a_choice: {
        if (definition->a_choice.name_hash == name_hash) {
          return definition;
        }
        break;
      }
      default: {
        ASSERT(false);
      }
    }
  }

  return 0;
}

struct OutputTypeResult {
  Bool ok;
  U32 main_size;
  U32 tag_size;
};

OutputTypeResult output_concrete_type(
  grammar::Root *in_root,
  Range<meta::StructDefinition> structs,
  Range<meta::ChoiceDefinition> choices,
  Range<UInt> assigned_indices,
  grammar::ConcreteType *in_concrete,
  meta::ConcreteType_tag *out_tag,
  meta::ConcreteType_payload *out_payload,
  Bool allow_tag,
  Bool force_size
) {
  switch (in_concrete->which) {
    case grammar::ConcreteType::Which::u8: {
      *out_tag = meta::ConcreteType_tag::u8;
      return { .ok = true, .main_size = 1 };
    }
    case grammar::ConcreteType::Which::u16: {
      *out_tag = meta::ConcreteType_tag::u16;
      return { .ok = true, .main_size = 2};
    }
    case grammar::ConcreteType::Which::u32: {
      *out_tag = meta::ConcreteType_tag::u32;
      return { .ok = true, .main_size = 4};
    }
    case grammar::ConcreteType::Which::u64: {
      *out_tag = meta::ConcreteType_tag::u64;
      return { .ok = true, .main_size = 8};
    }
    case grammar::ConcreteType::Which::i8: {
      *out_tag = meta::ConcreteType_tag::i8;
      return { .ok = true, .main_size = 1};
    }
    case grammar::ConcreteType::Which::i16: {
      *out_tag = meta::ConcreteType_tag::i16;
      return { .ok = true, .main_size = 2};
    }
    case grammar::ConcreteType::Which::i32: {
      *out_tag = meta::ConcreteType_tag::i32;
      return { .ok = true, .main_size = 4};
    }
    case grammar::ConcreteType::Which::i64: {
      *out_tag = meta::ConcreteType_tag::i64;
      return { .ok = true, .main_size = 8};
    }
    case grammar::ConcreteType::Which::f32: {
      *out_tag = meta::ConcreteType_tag::f32;
      return { .ok = true, .main_size = 4};
    }
    case grammar::ConcreteType::Which::f64: {
      *out_tag = meta::ConcreteType_tag::f64;
      return { .ok = true, .main_size = 8};
    }
    case grammar::ConcreteType::Which::zero_sized: {
      *out_tag = meta::ConcreteType_tag::zeroSized;
      return { .ok = true, .main_size = 0};
    }
    case grammar::ConcreteType::Which::defined: {
      auto definition = resolve_by_name_hash(
        in_root,
        in_concrete->defined.top_level_definition_name_hash
      );
      UInt ix = definition - in_root->definitions.pointer;
      ASSERT(ix < assigned_indices.count);
      if (definition->which == grammar::TopLevelDefinition::Which::a_struct) {
        auto struct_index = assigned_indices.pointer[ix];

        *out_tag = meta::ConcreteType_tag::definedStruct;
        out_payload->definedStruct = {
          .index = safe_int_cast<U32>(struct_index),
        };

        if (force_size) {
          return { .ok = true, .main_size = U32(-1) };
        }

        // We rely on the fact that this type has already been output.
        auto size = structs.pointer[struct_index].size;
        ASSERT(size > 0);
        return { .ok = true, .main_size = size };

      } else if (definition->which == grammar::TopLevelDefinition::Which::a_choice) {
        if (!allow_tag) {
          return {};
        }

        auto choice_index = assigned_indices.pointer[ix];

        *out_tag = meta::ConcreteType_tag::definedChoice;
        out_payload->definedChoice = {
          .index = safe_int_cast<U32>(choice_index),
        };

        if (force_size) {
          return { .ok = true, .main_size = U32(-1) };
        }

        // We rely on the fact that this type has already been output.
        auto payload_size = choices.pointer[choice_index].payloadSize;
        return {true, payload_size, SVFRT_TAG_SIZE};

      } else {
        ASSERT(false);
      }
    }
  }

  return {};
}

OutputTypeResult output_type(
  grammar::Root *in_root,
  Range<meta::StructDefinition> structs,
  Range<meta::ChoiceDefinition> choices,
  Range<UInt> assigned_indices,
  grammar::Type *in_type,
  meta::Type_tag *out_tag,
  meta::Type_payload *out_payload,
  Bool allow_tag
) {
  switch (in_type->which) {
    case grammar::Type::Which::concrete: {
      *out_tag = meta::Type_tag::concrete;
      return output_concrete_type(
        in_root,
        structs,
        choices,
        assigned_indices,
        &in_type->concrete.type,
        &out_payload->concrete.type_tag,
        &out_payload->concrete.type_payload,
        allow_tag,
        false // force_size
      );
    }
    case grammar::Type::Which::reference: {
      *out_tag = meta::Type_tag::reference;
      auto result = output_concrete_type(
        in_root,
        structs,
        choices,
        assigned_indices,
        &in_type->reference.type,
        &out_payload->reference.type_tag,
        &out_payload->reference.type_payload,
        false, // allow_tag
        true // force_size
      );
      result.main_size = sizeof(svf::Reference<void>);
      return result;
    }
    case grammar::Type::Which::sequence: {
      *out_tag = meta::Type_tag::sequence;
      auto result = output_concrete_type(
        in_root,
        structs,
        choices,
        assigned_indices,
        &in_type->sequence.element_type,
        &out_payload->sequence.elementType_tag,
        &out_payload->sequence.elementType_payload,
        false, // allow_tag
        true // force_size
      );
      result.main_size = sizeof(svf::Sequence<void>);
      return result;
    }
    default: {
      ASSERT(false);
    };
  }

  return {};
}

Bytes as_bytes(
  grammar::Root *in_root,
  vm::LinearArena *arena
) {
  auto start = vm::realign(arena);

  // First pass: count number of defined types.
  UInt num_structs = 0;
  UInt num_choices = 0;

  for (UInt i = 0; i < in_root->definitions.count; i++) {
    auto definition = in_root->definitions.pointer + i;
    if (definition->which == grammar::TopLevelDefinition::Which::a_struct) {
      num_structs++;
    } else if (definition->which == grammar::TopLevelDefinition::Which::a_choice) {
      num_choices++;
    } else {
      ASSERT(false);
    }
  }

  // Second pass: determine ordering.
  // Order of 0 means that the defined type does not depend on any other types.
  // Order of 1 means that the defined type only depends on order-0 types.
  // Et cetera.
  //
  // Also, assign struct/choice indices for top level definitions,
  // depending on their type.

  UInt next_struct_index = 0;
  UInt next_choice_index = 0;
  UInt definitions_done = 0;
  auto order_of_definitions = vm::many<UInt>(arena, in_root->definitions.count);
  auto assigned_orders = vm::many<UInt>(arena, in_root->definitions.count);
  auto assigned_indices = vm::many<UInt>(arena, in_root->definitions.count);
  for (UInt i = 0; i < assigned_orders.count; i++) {
    assigned_orders.pointer[i] = UInt(-1);
  }

  // TODO @performance: N^2.
  UInt current_order = 0;
  for (;;) {
    Bool all_ok = true;
    Bool some_ok = false;

    for (UInt i = 0; i < in_root->definitions.count; i++) {
      if (assigned_orders.pointer[i] < current_order) {
        continue;
      }

      Bool ok = true;
      auto definition = in_root->definitions.pointer + i;

      switch (definition->which) {
        case grammar::TopLevelDefinition::Which::a_struct: {
          for (UInt j = 0; j < definition->a_struct.fields.count; j++) {
            auto field = definition->a_struct.fields.pointer + j;
            if (field->type.which != grammar::Type::Which::concrete) {
              continue;
            }
            if (field->type.concrete.type.which != grammar::ConcreteType::Which::defined) {
              continue;
            }
            auto field_definition = resolve_by_name_hash(
              in_root,
              field->type.concrete.type.defined.top_level_definition_name_hash
            );
            if (!field_definition) {
              return {};
            }
            auto ix = field_definition - in_root->definitions.pointer;
            if (assigned_orders.pointer[ix] >= current_order) {
              ok = false;
              break;
            }
          }

          if (ok) {
            assigned_indices.pointer[i] = next_struct_index++;
          }

          break;
        }
        case grammar::TopLevelDefinition::Which::a_choice: {
          for (UInt j = 0; j < definition->a_choice.options.count; j++) {
            auto option = definition->a_choice.options.pointer + j;
            if (option->type.which != grammar::Type::Which::concrete) {
              continue;
            }
            if (option->type.concrete.type.which != grammar::ConcreteType::Which::defined) {
              continue;
            }
            auto option_definition = resolve_by_name_hash(
              in_root,
              option->type.concrete.type.defined.top_level_definition_name_hash
            );
            if (!option_definition) {
              return {};
            }
            auto ix = option_definition - in_root->definitions.pointer;
            if (assigned_orders.pointer[ix] >= current_order) {
              ok = false;
              break;
            }
          }

          if (ok) {
            assigned_indices.pointer[i] = next_choice_index++;
          }

          break;
        }
        default: {
          ASSERT(false);
        }
      }

      if (ok) {
        assigned_orders.pointer[i] = current_order;
        order_of_definitions.pointer[definitions_done++] = i;
        some_ok = true;
      } else {
        all_ok = false;
      }
    }

    if (all_ok) {
      break;
    }

    if (!some_ok) {
      return {};
    }

    current_order++;
  }

  // Third pass: fill in our output types.
  auto out_structs = vm::many<meta::StructDefinition>(arena, num_structs);
  auto out_choices = vm::many<meta::ChoiceDefinition>(arena, num_choices);

  for (U64 i = 0; i < order_of_definitions.count; i++) {
    auto ix = order_of_definitions.pointer[i];
    auto definition = in_root->definitions.pointer + ix;
    switch (definition->which) {
      case grammar::TopLevelDefinition::Which::a_struct: {
        auto in_struct = &definition->a_struct;
        auto out_struct = out_structs.pointer + assigned_indices.pointer[ix];

        auto out_fields = vm::many<meta::FieldDefinition>(
          arena,
          in_struct->fields.count
        );

        U32 size_sum = 0;
        for (U64 j = 0; j < in_struct->fields.count; j++) {
          auto in_field = in_struct->fields.pointer + j;
          auto out_field = out_fields.pointer + j;

          auto out_name = vm::many<U8>(arena, in_field->name.count);
          range_copy(out_name, in_field->name);

          *out_field = meta::FieldDefinition {
            .nameHash = in_field->name_hash,
            .name = {
              .data_offset_complement = ~offset_between<U32>(start, out_name.pointer),
              .count = safe_int_cast<U32>(out_name.count),
            },
            .offset = size_sum,
          };

          auto result = output_type(
            in_root,
            out_structs,
            out_choices,
            assigned_indices,
            &in_field->type,
            &out_field->type_tag,
            &out_field->type_payload,
            true // allow_tag
          );

          // TODO @proper-alignment.
          size_sum += result.main_size;
          size_sum += result.tag_size;

          if (!result.ok) {
            return {};
          };
        }
        if (size_sum == 0) {
          return {};
        }

        auto out_name = vm::many<U8>(arena, in_struct->name.count);
        range_copy(out_name, in_struct->name);

        *out_struct = meta::StructDefinition {
          .nameHash = in_struct->name_hash,
          .name = {
            .data_offset_complement = ~offset_between<U32>(start, out_name.pointer),
            .count = safe_int_cast<U32>(out_name.count),
          },
          .size = size_sum,
          .fields = {
            .data_offset_complement = ~offset_between<U32>(start, out_fields.pointer),
            .count = safe_int_cast<U32>(out_fields.count),
          },
        };

        break;
      }
      case grammar::TopLevelDefinition::Which::a_choice: {
        auto in_choice = &definition->a_choice;
        auto out_choice = out_choices.pointer + assigned_indices.pointer[ix];

        auto out_options = vm::many<meta::OptionDefinition>(
          arena,
          in_choice->options.count
        );

        U32 size_max = 0;
        for (U64 j = 0; j < in_choice->options.count; j++) {
          auto in_option = in_choice->options.pointer + j;
          auto out_option = out_options.pointer + j;

          auto out_name = vm::many<U8>(arena, in_option->name.count);
          range_copy(out_name, in_option->name);

          *out_option = meta::OptionDefinition {
            .nameHash = in_option->name_hash,
            .name = {
              .data_offset_complement = ~offset_between<U32>(start, out_name.pointer),
              .count = safe_int_cast<U32>(out_name.count),
            },
            .tag = safe_int_cast<U8>(j),
          };

          auto result = output_type(
            in_root,
            out_structs,
            out_choices,
            assigned_indices,
            &in_option->type,
            &out_option->type_tag,
            &out_option->type_payload,
            false // allow_tag
          );

          ASSERT(result.tag_size == 0);
          size_max = maxi(size_max, result.main_size);

          if (!result.ok) {
            return {};
          };
        }

        auto out_name = vm::many<U8>(arena, in_choice->name.count);
        range_copy(out_name, in_choice->name);

        *out_choice = meta::ChoiceDefinition {
          .nameHash = in_choice->name_hash,
          .name = {
            .data_offset_complement = ~offset_between<U32>(start, out_name.pointer),
            .count = safe_int_cast<U32>(out_name.count),
          },
          .payloadSize = size_max,
          .options = {
            .data_offset_complement = ~offset_between<U32>(start, out_options.pointer),
            .count = safe_int_cast<U32>(out_options.count),
          },
        };

        break;
      }
    }
  }

  auto out_name = vm::many<U8>(arena, in_root->schema_name.count);
  range_copy(out_name, in_root->schema_name);

  auto out_definition = vm::one<meta::SchemaDefinition>(arena);
  *out_definition = meta::SchemaDefinition {
    .nameHash = in_root->schema_name_hash,
    .name = {
      .data_offset_complement = ~offset_between<U32>(start, out_name.pointer),
      .count = safe_int_cast<U32>(out_name.count),
    },
    .structs = {
      .data_offset_complement = ~offset_between<U32>(start, out_structs.pointer),
      .count = safe_int_cast<U32>(out_structs.count),
    },
    .choices = {
      .data_offset_complement = ~offset_between<U32>(start, out_choices.pointer),
      .count = safe_int_cast<U32>(out_choices.count),
    },
  };

  auto end = arena->reserved_range.pointer + arena->waterline;
  return Bytes {
    .pointer = (Byte *) start,
    .count = offset_between<U64>(start, end),
  };
}

} // namespace generation
