#include <cstdlib>
#include <src/library.hpp>
#include <src/svf_runtime.h>
#include "../core.hpp"

namespace core::generation {

struct NameMappingToWrite {
  U64 id;
  Range<U8> name;

  svf::runtime::Sequence<U8> sequence;
  Bool is_duplicate;
};

int compare_by_id(const void *va, const void *vb) {
  auto a = (NameMappingToWrite *) va;
  auto b = (NameMappingToWrite *) vb;
  if (a->id < b->id) {
    return -1;
  } else if (a->id > b->id) {
    return 1;
  } else {
    return 0;
  }
}

// TODO @performance: N^2.
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
        return UNREACHABLE;
      }
    }
  }

  return 0;
}

struct OutputTypeResult {
  FailCode fail_code;
  U32 main_size;
  U32 tag_size;
};

OutputTypeResult output_concrete_type(
  grammar::Root *in_root,
  Range<Meta::StructDefinition> structs,
  Range<Meta::ChoiceDefinition> choices,
  Range<UInt> assigned_indices,
  grammar::ConcreteType *in_concrete,
  Meta::ConcreteType_tag *out_tag,
  Meta::ConcreteType_payload *out_payload,
  Bool allow_tag,
  Bool force_size
) {
  switch (in_concrete->which) {
    case grammar::ConcreteType::Which::u8: {
      *out_tag = Meta::ConcreteType_tag::u8;
      return { .main_size = 1 };
    }
    case grammar::ConcreteType::Which::u16: {
      *out_tag = Meta::ConcreteType_tag::u16;
      return { .main_size = 2};
    }
    case grammar::ConcreteType::Which::u32: {
      *out_tag = Meta::ConcreteType_tag::u32;
      return { .main_size = 4};
    }
    case grammar::ConcreteType::Which::u64: {
      *out_tag = Meta::ConcreteType_tag::u64;
      return { .main_size = 8};
    }
    case grammar::ConcreteType::Which::i8: {
      *out_tag = Meta::ConcreteType_tag::i8;
      return { .main_size = 1};
    }
    case grammar::ConcreteType::Which::i16: {
      *out_tag = Meta::ConcreteType_tag::i16;
      return { .main_size = 2};
    }
    case grammar::ConcreteType::Which::i32: {
      *out_tag = Meta::ConcreteType_tag::i32;
      return { .main_size = 4};
    }
    case grammar::ConcreteType::Which::i64: {
      *out_tag = Meta::ConcreteType_tag::i64;
      return { .main_size = 8};
    }
    case grammar::ConcreteType::Which::f32: {
      *out_tag = Meta::ConcreteType_tag::f32;
      return { .main_size = 4};
    }
    case grammar::ConcreteType::Which::f64: {
      *out_tag = Meta::ConcreteType_tag::f64;
      return { .main_size = 8};
    }
    case grammar::ConcreteType::Which::zero_sized: {
      *out_tag = Meta::ConcreteType_tag::zeroSized;
      return { .main_size = 0};
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

        *out_tag = Meta::ConcreteType_tag::definedStruct;
        out_payload->definedStruct = {
          .index = safe_int_cast<U32>(struct_index),
        };

        if (force_size) {
          return { .main_size = U32(-1) };
        }

        // We rely on the fact that this type has already been output.
        auto size = structs.pointer[struct_index].size;
        ASSERT(size > 0);
        return {
          .main_size = size
        };
      } else if (definition->which == grammar::TopLevelDefinition::Which::a_choice) {
        if (!allow_tag) {
          return {
            .fail_code = FailCode::choice_not_allowed,
          };
        }

        auto choice_index = assigned_indices.pointer[ix];

        *out_tag = Meta::ConcreteType_tag::definedChoice;
        out_payload->definedChoice = {
          .index = safe_int_cast<U32>(choice_index),
        };

        if (force_size) {
          return { .main_size = U32(-1) };
        }

        // We rely on the fact that this type has already been output.
        auto payload_size = choices.pointer[choice_index].payloadSize;

        return {
          .main_size = payload_size,
          .tag_size = SVFRT_TAG_SIZE,
        };
      }
    }
  }

  return UNREACHABLE;
}

OutputTypeResult output_type(
  grammar::Root *in_root,
  Range<Meta::StructDefinition> structs,
  Range<Meta::ChoiceDefinition> choices,
  Range<UInt> assigned_indices,
  grammar::Type *in_type,
  Meta::Type_tag *out_tag,
  Meta::Type_payload *out_payload,
  Bool allow_tag
) {
  switch (in_type->which) {
    case grammar::Type::Which::concrete: {
      *out_tag = Meta::Type_tag::concrete;
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
      *out_tag = Meta::Type_tag::reference;
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
      result.main_size = sizeof(svf::runtime::Reference<void>);
      return result;
    }
    case grammar::Type::Which::sequence: {
      *out_tag = Meta::Type_tag::sequence;
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
      result.main_size = sizeof(svf::runtime::Sequence<void>);
      return result;
    }
  }

  return UNREACHABLE;
}

GenerationResult as_bytes(
  grammar::Root *in_root,
  vm::LinearArena *arena,
  vm::LinearArena *arena2
) {
  // TODO: check struct/choice ID for collisions. Currently, multiple
  // definitions with the same ID are accepted, which is not great.

  // This arena is used to write the message.
  auto start_arena = vm::realign(arena);
  // This arena is used to accumulate name mappings.
  auto start_arena2_map = vm::realign(arena2);

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
      return UNREACHABLE;
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
              return {
                .fail_code = FailCode::type_not_found,
              };
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
              return {
                .fail_code = FailCode::type_not_found,
              };
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
          return UNREACHABLE;
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
      return {
        .fail_code = FailCode::cyclical_dependency,
      };
    }

    current_order++;
  }

  // Third pass: fill in our output types.
  auto out_structs = vm::many<Meta::StructDefinition>(arena, num_structs);
  auto out_choices = vm::many<Meta::ChoiceDefinition>(arena, num_choices);

  for (U64 i = 0; i < order_of_definitions.count; i++) {
    auto ix = order_of_definitions.pointer[i];
    auto definition = in_root->definitions.pointer + ix;
    switch (definition->which) {
      case grammar::TopLevelDefinition::Which::a_struct: {
        auto in_struct = &definition->a_struct;
        auto out_struct = out_structs.pointer + assigned_indices.pointer[ix];

        auto out_fields = vm::many<Meta::FieldDefinition>(
          arena,
          in_struct->fields.count
        );

        U32 size_sum = 0;
        for (U64 j = 0; j < in_struct->fields.count; j++) {
          auto in_field = in_struct->fields.pointer + j;
          auto out_field = out_fields.pointer + j;

          auto mapping = vm::one<NameMappingToWrite>(arena2);
          mapping->id = in_field->name_hash;
          mapping->name = in_field->name;

          *out_field = Meta::FieldDefinition {
            .fieldId = in_field->name_hash,
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

          // TODO @proper-alignment: tags.
          size_sum += result.main_size;
          size_sum += result.tag_size;

          if (result.fail_code != FailCode::ok) {
            return {
              .fail_code = result.fail_code,
            };
          };
        }

        if (size_sum == 0) {
          return {
            .fail_code = FailCode::empty_struct,
          };
        }

        auto mapping = vm::one<NameMappingToWrite>(arena2);
        mapping->id = in_struct->name_hash;
        mapping->name = in_struct->name;

        *out_struct = Meta::StructDefinition {
          .typeId = in_struct->name_hash,
          .size = size_sum,
          .fields = {
            .data_offset_complement = ~offset_between<U32>(start_arena, out_fields.pointer),
            .count = safe_int_cast<U32>(out_fields.count),
          },
        };

        break;
      }
      case grammar::TopLevelDefinition::Which::a_choice: {
        auto in_choice = &definition->a_choice;
        auto out_choice = out_choices.pointer + assigned_indices.pointer[ix];

        auto out_options = vm::many<Meta::OptionDefinition>(
          arena,
          in_choice->options.count
        );

        U32 size_max = 0;
        for (U64 j = 0; j < in_choice->options.count; j++) {
          auto in_option = in_choice->options.pointer + j;
          auto out_option = out_options.pointer + j;

          auto mapping = vm::one<NameMappingToWrite>(arena2);
          mapping->id = in_option->name_hash;
          mapping->name = in_option->name;

          *out_option = Meta::OptionDefinition {
            .optionId = in_option->name_hash,
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

          if (result.fail_code != FailCode::ok) {
            return {
              .fail_code = result.fail_code,
            };
          };
        }

        if (in_choice->options.count == 0) {
          return {
            .fail_code = FailCode::empty_choice,
          };
        }

        auto mapping = vm::one<NameMappingToWrite>(arena2);
        mapping->id = in_choice->name_hash;
        mapping->name = in_choice->name;

        *out_choice = Meta::ChoiceDefinition {
          .typeId = in_choice->name_hash,
          .payloadSize = size_max,
          .options = {
            .data_offset_complement = ~offset_between<U32>(start_arena, out_options.pointer),
            .count = safe_int_cast<U32>(out_options.count),
          },
        };

        break;
      }
    }
  }

  auto mapping = vm::one<NameMappingToWrite>(arena2);
  mapping->id = in_root->schema_name_hash;
  mapping->name = in_root->schema_name;

  auto out_definition = vm::one<Meta::SchemaDefinition>(arena);
  *out_definition = Meta::SchemaDefinition {
    .schemaId = in_root->schema_name_hash,
    .structs = {
      .data_offset_complement = ~offset_between<U32>(start_arena, out_structs.pointer),
      .count = safe_int_cast<U32>(out_structs.count),
    },
    .choices = {
      .data_offset_complement = ~offset_between<U32>(start_arena, out_choices.pointer),
      .count = safe_int_cast<U32>(out_choices.count),
    },
  };

  auto end_arena = arena->reserved_range.pointer + arena->waterline;
  auto end_arena2_map = arena2->reserved_range.pointer + arena2->waterline;

  auto names = Range<NameMappingToWrite> {
    .pointer = (NameMappingToWrite *) start_arena2_map,
    .count = (end_arena2_map - (Byte *) start_arena2_map) / sizeof(NameMappingToWrite),
  };

  qsort(names.pointer, names.count, sizeof(*names.pointer), compare_by_id);

  // This arena is now used to write the appendix.
  auto start_arena2_appendix = vm::realign(arena2);

  UInt duplicate_names = 0;
  for (UInt i = 0; i < names.count; i++) {
    auto item = names.pointer + i;

    if (i > 0) {
      auto prev = names.pointer + i - 1;
      if (prev->id == item->id) {
        auto is_same_name = range_equal(prev->name, item->name);
        if (!is_same_name) {
          return {
            .fail_code = FailCode::name_collision,
          };
        }

        duplicate_names++;
        item->is_duplicate = true;
        continue;
      }
    }

    auto out_name = vm::many<U8>(arena2, item->name.count);
    range_copy(out_name, item->name);
    item->sequence = {
      .data_offset_complement = ~offset_between<U32>(start_arena2_appendix, out_name.pointer),
      .count = safe_int_cast<U32>(out_name.count),
    };
  }

  auto out_names = vm::many<Meta::NameMapping>(arena2, names.count - duplicate_names);

  for (UInt i = 0, j = 0; i < names.count; i++) {
    auto item = names.pointer + i;
    if (item->is_duplicate) {
      continue;
    }
    out_names.pointer[j].id = item->id;
    out_names.pointer[j].name = item->sequence;
    j++;
  }

  auto out_appendix = vm::one<Meta::Appendix>(arena2);
  out_appendix->names = {
    .data_offset_complement = ~offset_between<U32>(start_arena2_appendix, out_names.pointer),
    .count = safe_int_cast<U32>(out_names.count),
  };

  auto end_arena2_appendix = arena2->reserved_range.pointer + arena2->waterline;

  return GenerationResult {
    .schema = {
      .pointer = (Byte *) start_arena,
      .count = offset_between<U64>(start_arena, end_arena),
    },
    .appendix = {
      .pointer = (Byte *) start_arena2_appendix,
      .count = offset_between<U64>(start_arena2_appendix, end_arena2_appendix),
    },
  };
}

} // namespace core::generation
