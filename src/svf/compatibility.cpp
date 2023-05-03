// UNREVIEWED.
#include "../platform.hpp"
#include "../../meta/meta.hpp"

namespace svf::compatiblity::binary {

namespace meta = svf::svf_meta;

template<typename T>
inline static
Range<T> to_range(Range<Byte> data_range, svf::Array<T> array) {
  // Make sure the array is within the data range.
  if (U64(array.pointer_offset) + U64(array.count) * sizeof(T) > data_range.count) {
    return {};
  }

  return {
    .pointer = (T*) (data_range.pointer + array.pointer_offset),
    .count = array.count,
  };
}

struct CheckContext {
  Range<Byte> r0;
  Range<Byte> r1;
  meta::Schema* s0;
  meta::Schema* s1;
};

Bool check_struct(
  CheckContext *ctx,
  meta::StructDefinition *s0,
  meta::StructDefinition *s1
);

Bool check_choice(
  CheckContext *ctx,
  meta::ChoiceDefinition *c0,
  meta::ChoiceDefinition *c1
);

meta::StructDefinition *find_struct_by_name_hash(
  Range<meta::StructDefinition> structs,
  U64 name_hash
) {
  for (U64 i = 0; i < structs.count; i++) {
    auto struct_definition = structs.pointer + i;
    if (struct_definition->name_hash == name_hash) {
      return struct_definition;
    }
  }

  return {};
}

meta::ChoiceDefinition *find_choice_by_name_hash(
  Range<meta::ChoiceDefinition> choices,
  U64 name_hash
) {
  for (U64 i = 0; i < choices.count; i++) {
    auto choice_definition = choices.pointer + i;
    if (choice_definition->name_hash == name_hash) {
      return choice_definition;
    }
  }

  return {};
}

Bool check_concrete_type(
  CheckContext *ctx,
  meta::ConcreteType_enum t0,
  meta::ConcreteType_enum t1,
  meta::ConcreteType_union *u0,
  meta::ConcreteType_union *u1
) {
  if (t0 != t1) {
    return false;
  }

  auto structs0 = to_range(ctx->r0, ctx->s0->structs);
  auto structs1 = to_range(ctx->r1, ctx->s1->structs);
  auto choices0 = to_range(ctx->r0, ctx->s0->choices);
  auto choices1 = to_range(ctx->r1, ctx->s1->choices);

  if (t0 == meta::ConcreteType_enum::defined) {
    auto struct0 = find_struct_by_name_hash(structs0, u0->defined.top_level_definition_name_hash);
    auto struct1 = find_struct_by_name_hash(structs1, u1->defined.top_level_definition_name_hash);
    auto choice0 = find_choice_by_name_hash(choices0, u0->defined.top_level_definition_name_hash);
    auto choice1 = find_choice_by_name_hash(choices1, u1->defined.top_level_definition_name_hash);
    if (struct0 && struct1) {
      return check_struct(ctx, struct0, struct1);
    } else if (choice0 && choice1) {
      return check_choice(ctx, choice0, choice1);
    } else {
      return false;
    }
  }

  return true;
}

Bool check_type(
  CheckContext *ctx,
  meta::Type_enum t0,
  meta::Type_enum t1,
  meta::Type_union *u0,
  meta::Type_union *u1
) {
  if (t0 != t1) {
    return false;
  }

  if (t0 == meta::Type_enum::pointer) {
    return check_concrete_type(
      ctx,
      u0->pointer.type_enum,
      u1->pointer.type_enum,
      &u0->pointer.type_union,
      &u1->pointer.type_union
    );
  }

  if (t0 == meta::Type_enum::flexible_array) {
    return check_concrete_type(
      ctx,
      u0->flexible_array.element_type_enum,
      u1->flexible_array.element_type_enum,
      &u0->flexible_array.element_type_union,
      &u1->flexible_array.element_type_union
    );
  }

  if (t0 == meta::Type_enum::concrete) {
    return check_concrete_type(
      ctx,
      u0->concrete.type_enum,
      u1->concrete.type_enum,
      &u0->concrete.type_union,
      &u1->concrete.type_union
    );
  }

  ASSERT(false);
  return false;
}

Bool check_struct(
  CheckContext *ctx,
  meta::StructDefinition *s0,
  meta::StructDefinition *s1
) {
  // all `s0` fields must be present in `s1` in the same order,
  // and their types be compatible.

  auto fields0 = to_range(ctx->r0, s0->fields);
  auto fields1 = to_range(ctx->r1, s1->fields);

  if (fields1.count < fields0.count) {
    return false;
  }

  for (U64 i = 0; i < fields0.count; i++) {
    auto field0 = fields0.pointer + i;
    auto field1 = fields1.pointer + i;

    if (field0->name_hash != field1->name_hash) {
      return false;
    }

    if (!check_type(
        ctx,
        field0->type_enum,
        field1->type_enum,
        &field0->type_union,
        &field1->type_union
      )) {
      return false;
    }
  }

  return true;
}

Bool check_choice(
  CheckContext *ctx,
  meta::ChoiceDefinition *c0,
  meta::ChoiceDefinition *c1
) {
  // all `c1` options must be present in `c0` (same name_hash + index),
  // and their types be compatible.

  auto options0 = to_range(ctx->r0, c0->options);
  auto options1 = to_range(ctx->r1, c1->options);

  if (options0.count < options1.count) {
    return false;
  }

  for (U64 i = 0; i < options1.count; i++) {
    auto option1 = options1.pointer + i;

    Bool found = false;
    for (U64 j = 0; j < options0.count; j++) {
      auto option0 = options0.pointer + j;

      if (1
        && option0->name_hash == option1->name_hash
        && option0->index == option1->index
      ) {
        found = true;
        if (!check_type(
            ctx,
            option0->type_enum,
            option1->type_enum,
            &option0->type_union,
            &option1->type_union
          )) {
          return false;
        }
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

U64 const INVALID = U64(-1);

// TODO: recursive types will break this!
Bool check_entry(
  CheckContext *ctx,
  U64 entry_name_hash
) {
  U64 struct_index0 = INVALID;
  U64 struct_index1 = INVALID;
  U64 choice_index0 = INVALID;
  U64 choice_index1 = INVALID;

  auto structs0 = to_range(ctx->r0, ctx->s0->structs);
  auto structs1 = to_range(ctx->r1, ctx->s1->structs);
  auto choices0 = to_range(ctx->r0, ctx->s0->choices);
  auto choices1 = to_range(ctx->r1, ctx->s1->choices);

  for (U64 i = 0; i < structs0.count; i++) {
    if (structs0.pointer[i].name_hash == entry_name_hash) {
      struct_index0 = i;
      break;
    }
  }

  for (U64 i = 0; i < structs1.count; i++) {
    if (structs1.pointer[i].name_hash == entry_name_hash) {
      struct_index1 = i;
      break;
    }
  }

  for (U64 i = 0; i < choices0.count; i++) {
    if (choices0.pointer[i].name_hash == entry_name_hash) {
      choice_index0 = i;
      break;
    }
  }

  for (U64 i = 0; i < choices1.count; i++) {
    if (choices1.pointer[i].name_hash == entry_name_hash) {
      choice_index1 = i;
      break;
    }
  }

  if (struct_index0 != INVALID && struct_index1 != INVALID) {
    if (choice_index0 != INVALID || choice_index1 != INVALID) {
      return false;
    }
    return check_struct(
      ctx,
      structs0.pointer + struct_index0,
      structs1.pointer + struct_index1
    );
  }

  if (choice_index0 != INVALID && choice_index1 != INVALID) {
    if (struct_index0 != INVALID || struct_index1 != INVALID) {
      return false;
    }
    return check_choice(
      ctx,
      choices0.pointer + choice_index0,
      choices1.pointer + choice_index1
    );
  }

  return false;
}

} // namespace svf::compatibility::binary