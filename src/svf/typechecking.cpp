#include <cstdlib>
#include "../platform.hpp"
#include "grammar.hpp"

namespace svf::typechecking {

using namespace grammar;

bool resolve_type(Root *root, Type *type) {
  ConcreteType *concrete_type;
  switch (type->which) {
    case Type::Which::concrete: {
      concrete_type = &type->concrete.type;
      break;
    }
    case Type::Which::pointer: {
      concrete_type = &type->pointer.type;
      break;
    }
    case Type::Which::flexible_array: {
      concrete_type = &type->flexible_array.element_type;
      break;
    }
    default: {
      ASSERT(false);
    }
  }

  if (concrete_type->which == ConcreteType::Which::unresolved) {
    for (size_t k = 0; k < root->definitions.count; k++) {
      auto definition = root->definitions.pointer + k;
      switch (definition->which) {
        case TopLevelDefinition::Which::a_struct: {
          if (definition->a_struct.name_hash == concrete_type->unresolved.name_hash) {
            concrete_type->which = ConcreteType::Which::defined,
            concrete_type->defined = {
              .top_level_definition_index = safe_int_cast<U32>(k),
            };
            return true;
          }
          break;
        }
        case TopLevelDefinition::Which::a_choice: {
          if (definition->a_choice.name_hash == concrete_type->unresolved.name_hash) {
            concrete_type->which = ConcreteType::Which::defined,
            concrete_type->defined = {
              .top_level_definition_index = safe_int_cast<U32>(k),
            };
            return true;
          }
          break;
        }
        default: {
          ASSERT(false);
        }
      }
    }
    return false;
  }

  return true;
}

bool resolve_types(Root *root) {
  for (size_t i = 0; i < root->definitions.count; i++) {
    auto definition = root->definitions.pointer + i;
    switch (definition->which) {
      case TopLevelDefinition::Which::a_struct: {
        auto struct_definition = &definition->a_struct;
        for (size_t j = 0; j < struct_definition->fields.count; j++) {
          auto field = struct_definition->fields.pointer + j;
          if (!resolve_type(root, &field->type)) {
            return false;
          }
        }
        break;
      }
      case TopLevelDefinition::Which::a_choice: {
        auto choice_definition = &definition->a_choice;
        for (size_t j = 0; j < choice_definition->options.count; j++) {
          auto option = choice_definition->options.pointer + j;
          if (!resolve_type(root, &option->type)) {
            return false;
          }
        }
        break;
      }
      default: {
        ASSERT(false);
      }
    }
  }
  return true;
}

int compare_by_ordering(const void *a_, const void *b_) {
  auto a = (OrderElement *) a_;
  auto b = (OrderElement *) b_;
  if (a->ordering < b->ordering) {
    return -1;
  } else if (a->ordering > b->ordering) {
    return 1;
  } else {
    return 0;
  }
}

Range<OrderElement> order_types(grammar::Root *root, vm::LinearArena *arena) {
  auto result = vm::allocate_many<OrderElement>(arena, root->definitions.count);
  ASSERT(result.pointer);

  for (size_t i = 0; i < root->definitions.count; i++) {
    result.pointer[i] = {
      .index = i,
      .ordering = size_t(-1),
    };
  }

  size_t current_ordering = 0;

  for (;;) {
    bool all_ok = true;
    bool some_ok = false;

    for (size_t i = 0; i < root->definitions.count; i++) {
      if (result.pointer[i].ordering < current_ordering) {
        continue;
      }
      bool ok = true;
      auto definition = root->definitions.pointer + i;
      switch (definition->which) {
        case grammar::TopLevelDefinition::Which::a_struct: {
          for (size_t j = 0; j < definition->a_struct.fields.count; j++) {
            auto field = definition->a_struct.fields.pointer + j;
            if (field->type.which != grammar::Type::Which::concrete) {
              continue;
            }
            if (field->type.concrete.type.which != grammar::ConcreteType::Which::defined) {
              continue;
            }
            auto ix = field->type.concrete.type.defined.top_level_definition_index;
            if (result.pointer[ix].ordering >= current_ordering) {
              ok = false;
              break;
            }
          }
          break;
        }
        case grammar::TopLevelDefinition::Which::a_choice: {
          for (size_t j = 0; j < definition->a_choice.options.count; j++) {
            auto option = definition->a_choice.options.pointer + j;
            if (option->type.which != grammar::Type::Which::concrete) {
              continue;
            }
            if (option->type.concrete.type.which != grammar::ConcreteType::Which::defined) {
              continue;
            }
            auto ix = option->type.concrete.type.defined.top_level_definition_index;
            if (result.pointer[ix].ordering >= current_ordering) {
              ok = false;
              break;
            }
          }
          break;
        }
        default: {
          ASSERT(false);
        }
      }
      if (ok) {
        some_ok = true;
        result.pointer[i].ordering = current_ordering;
      } else {
        all_ok = false;
      }
    }

    if (all_ok) {
      break;
    }

    if (some_ok) {
      current_ordering++;
    } else {
      return {};
    }
  }

  qsort(
    result.pointer,
    result.count,
    sizeof(*result.pointer),
    compare_by_ordering
  );

  return result;
}

} // namespace typechecking