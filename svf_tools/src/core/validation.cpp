#include <src/library.hpp>
#include "../core.hpp"
#include "common.hpp"

namespace core::validation {

bool has_higher_dependencies(
  Range<UInt> assigned_struct_orders,
  Range<UInt> assigned_choice_orders,
  UInt current_order,
  Meta::Type_tag in_tag,
  Meta::Type_payload *in_payload
) {
  if (in_tag == Meta::Type_tag::concrete) {
    auto type = in_payload->concrete.type_tag;
    if (type == Meta::ConcreteType_tag::definedStruct) {
      auto it = &in_payload->concrete.type_payload.definedStruct;
      if (assigned_struct_orders.pointer[it->index] >= current_order) {
        return true;
      }
    } else if (type == Meta::ConcreteType_tag::definedChoice) {
      auto it = &in_payload->concrete.type_payload.definedChoice;
      if (assigned_choice_orders.pointer[it->index] >= current_order) {
        return true;
      }
    }
  }

  return false;
}

Result validate(vm::LinearArena *arena, Bytes schema_bytes) {
  // TODO: this is mostly a placeholder for now. The reason for separation
  // between this and "generation.cpp" is that this is to also be exposed as a
  // separate `svfc` subcommand, which would take a completely arbitrary schema
  // as an input. We need to check _everything_ here, and right now, the only
  // thing checked is cyclical dependencies.

  ASSERT(schema_bytes.count >= sizeof(Meta::SchemaDefinition));

  // TODO: @proper-alignment.
  auto schema_definition = (Meta::SchemaDefinition *) (
    schema_bytes.pointer +
    schema_bytes.count -
    sizeof(Meta::SchemaDefinition)
  );

  auto structs = to_range(schema_bytes, schema_definition->structs);
  auto choices = to_range(schema_bytes, schema_definition->choices);

  UInt current_order = 0;
  auto assigned_struct_orders = vm::many<UInt>(arena, structs.count);
  auto assigned_choice_orders = vm::many<UInt>(arena, choices.count);
  for (UInt i = 0; i < assigned_struct_orders.count; i++) {
    assigned_struct_orders.pointer[i] = UInt(-1);
  }
  for (UInt i = 0; i < assigned_choice_orders.count; i++) {
    assigned_choice_orders.pointer[i] = UInt(-1);
  }

  auto ordering = vm::many<TLDRef>(arena, structs.count + choices.count);
  UInt ordering_done = 0;

  for (;;) {
    Bool all_ok = true;
    Bool some_ok = false;

    for (UInt i = 0; i < structs.count; i++) {
      Bool ok = true;
      auto a_struct = structs.pointer + i;
      if (assigned_struct_orders.pointer[i] < current_order) {
        continue;
      }

      auto fields = to_range(schema_bytes, a_struct->fields);
      for (UInt j = 0; j < fields.count; j++) {
        auto field = fields.pointer + j;
        if (has_higher_dependencies(
          assigned_struct_orders,
          assigned_choice_orders,
          current_order,
          field->type_tag,
          &field->type_payload
        )) {
          ok = false;
          break;
        }
      }

      if (ok) {
        assigned_struct_orders.pointer[i] = current_order;
        ordering.pointer[ordering_done++] = TLDRef {
          .index = i,
          .type = Meta::ConcreteType_tag::definedStruct,
        };
        some_ok = true;
      } else {
        all_ok = false;
      }
    }

    for (UInt i = 0; i < choices.count; i++) {
      Bool ok = true;
      auto a_choice = choices.pointer + i;
      if (assigned_choice_orders.pointer[i] < current_order) {
        continue;
      }

      auto options = to_range(schema_bytes, a_choice->options);
      for (UInt j = 0; j < options.count; j++) {
        auto option = options.pointer + j;
        if (has_higher_dependencies(
          assigned_struct_orders,
          assigned_choice_orders,
          current_order,
          option->type_tag,
          &option->type_payload
        )) {
          ok = false;
          break;
        }
      }

      if (ok) {
        assigned_choice_orders.pointer[i] = current_order;
        ordering.pointer[ordering_done++] = TLDRef {
          .index = i,
          .type = Meta::ConcreteType_tag::definedChoice,
        };
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

  ASSERT(ordering_done == structs.count + choices.count);

  return Result {
    .valid = true,
    .ordering = ordering,
  };
}

} // namespace core::validation
