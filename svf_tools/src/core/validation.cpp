#include <src/library.hpp>
#include "../core.hpp"
#include "common.hpp"

namespace core::validation {

namespace meta = svf::META;

bool has_higher_dependencies(
  Range<UInt> assigned_struct_orders,
  Range<UInt> assigned_choice_orders,
  UInt current_order,
  meta::Type_enum in_enum,
  meta::Type_union *in_union
) {
  if (in_enum == meta::Type_enum::concrete) {
    auto type = in_union->concrete.type_enum;
    if (type == meta::ConcreteType_enum::defined_struct) {
      auto it = &in_union->concrete.type_union.defined_struct;
      if (assigned_struct_orders.pointer[it->index] >= current_order) {
        return true;
      }
    } else if (type == meta::ConcreteType_enum::defined_choice) {
      auto it = &in_union->concrete.type_union.defined_choice;
      if (assigned_choice_orders.pointer[it->index] >= current_order) {
        return true;
      }
    }
  }

  return false;
}

Result validate(vm::LinearArena *arena, Bytes schema_bytes) {
  // TODO @proper-alignment.
  auto in_schema = (meta::Schema *) (
    schema_bytes.pointer +
    schema_bytes.count -
    sizeof(meta::Schema)
  );

  auto structs = to_range(schema_bytes, in_schema->structs);
  auto choices = to_range(schema_bytes, in_schema->choices);

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
          field->type_enum,
          &field->type_union
        )) {
          ok = false;
          break;
        }
      }

      if (ok) {
        assigned_struct_orders.pointer[i] = current_order;
        ordering.pointer[ordering_done++] = TLDRef {
          .index = i,
          .type = meta::ConcreteType_enum::defined_struct,
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
          option->type_enum,
          &option->type_union
        )) {
          ok = false;
          break;
        }
      }

      if (ok) {
        assigned_choice_orders.pointer[i] = current_order;
        ordering.pointer[ordering_done++] = TLDRef {
          .index = i,
          .type = meta::ConcreteType_enum::defined_choice,
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
