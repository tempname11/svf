#ifndef SVFRT_SINGLE_FILE
  #include "svf_runtime.h"
  #include "svf_internal.h"
#endif

typedef struct SVFRT_CheckContext {
  // The entry structure for both schemas. We are going to check the "backward"
  // (write Schema 0, then read Schema 1) compatibility.
  SVF_META_Schema* s0;
  SVF_META_Schema* s1;

  // Byte ranges of the two schemas.
  SVFRT_Bytes r0;
  SVFRT_Bytes r1;

  // What do Schema 1 structs/choices match to in Schema 0?
  SVFRT_RangeU32 s1_struct_matches;
  SVFRT_RangeU32 s1_choice_matches;

  // What stride should be used when indexing a sequence of structs? With binary
  // compatibility, it is not size of the Schema 1 struct, even though we are
  // accessing the data through it. It is actually the size of the Schema 0
  // struct, since that's what the original sequence was using as the stride.
  SVFRT_RangeU32 s1_struct_strides;

  // Lowest level seen so far. Should be >= required level.
  SVFRT_CompatibilityLevel current_level;

  // Required level. We can exit early if we see that something does not match it.
  SVFRT_CompatibilityLevel required_level;
} SVFRT_CheckContext;

bool SVFRT_check_struct(
  SVFRT_CheckContext *ctx,
  uint32_t s0_index,
  uint32_t s1_index
);

bool SVFRT_check_choice(
  SVFRT_CheckContext *ctx,
  uint32_t s0_index,
  uint32_t s1_index
);

bool check_concrete_type(
  SVFRT_CheckContext *ctx,
  SVF_META_ConcreteType_enum t0,
  SVF_META_ConcreteType_enum t1,
  SVF_META_ConcreteType_union *u0,
  SVF_META_ConcreteType_union *u1
) {
  if (t0 == t1) {
    if (t0 == SVF_META_ConcreteType_defined_struct) {
      return SVFRT_check_struct(ctx, u0->defined_struct.index, u1->defined_struct.index);
    } else if (t0 == SVF_META_ConcreteType_defined_choice) {
      return SVFRT_check_choice(ctx, u0->defined_choice.index, u1->defined_choice.index);
    }

    // Other types are primitive.
    return true;
  }

  if (ctx->required_level <= SVFRT_compatibility_logical) {
    switch (t1) {
      case SVF_META_ConcreteType_u16: {
        return t0 == SVF_META_ConcreteType_u8;
      }
      case SVF_META_ConcreteType_u32: {
        return (0
          || (t0 == SVF_META_ConcreteType_u16)
          || (t0 == SVF_META_ConcreteType_u8)
        );
      }
      case SVF_META_ConcreteType_u64: {
        return (0
          || (t0 == SVF_META_ConcreteType_u32)
          || (t0 == SVF_META_ConcreteType_u16)
          || (t0 == SVF_META_ConcreteType_u8)
        );
      }
      case SVF_META_ConcreteType_i16: {
        return (0
          || (t0 == SVF_META_ConcreteType_i8)
          || (t0 == SVF_META_ConcreteType_u8)
        );
      }
      case SVF_META_ConcreteType_i32: {
        return (0
          || (t0 == SVF_META_ConcreteType_i16)
          || (t0 == SVF_META_ConcreteType_i8)
          || (t0 == SVF_META_ConcreteType_u16)
          || (t0 == SVF_META_ConcreteType_u8)
        );
      }
      case SVF_META_ConcreteType_i64: {
        return (0
          || (t0 == SVF_META_ConcreteType_i32)
          || (t0 == SVF_META_ConcreteType_i16)
          || (t0 == SVF_META_ConcreteType_i8)
          || (t0 == SVF_META_ConcreteType_u32)
          || (t0 == SVF_META_ConcreteType_u16)
          || (t0 == SVF_META_ConcreteType_u8)
        );
      }
      case SVF_META_ConcreteType_f64: {
        return t0 == SVF_META_ConcreteType_f32;
      }
      default: {
        return false;
      }
    }
  }

  return false;
}

bool check_type(
  SVFRT_CheckContext *ctx,
  SVF_META_Type_enum t0,
  SVF_META_Type_enum t1,
  SVF_META_Type_union *u0,
  SVF_META_Type_union *u1
) {
  if (t0 != t1) {
    return false;
  }

  if (t0 == SVF_META_Type_reference) {
    return check_concrete_type(
      ctx,
      u0->reference.type_enum,
      u1->reference.type_enum,
      &u0->reference.type_union,
      &u1->reference.type_union
    );
  }

  if (t0 == SVF_META_Type_sequence) {
    return check_concrete_type(
      ctx,
      u0->sequence.element_type_enum,
      u1->sequence.element_type_enum,
      &u0->sequence.element_type_union,
      &u1->sequence.element_type_union
    );
  }

  if (t0 == SVF_META_Type_concrete) {
    return check_concrete_type(
      ctx,
      u0->concrete.type_enum,
      u1->concrete.type_enum,
      &u0->concrete.type_union,
      &u1->concrete.type_union
    );
  }

  // Internal error.
  return false;
}

bool SVFRT_check_struct(
  SVFRT_CheckContext *ctx,
  uint32_t s0_index,
  uint32_t s1_index
) {
  uint32_t match = ctx->s1_struct_matches.pointer[s1_index];
  if (match == s0_index) {
    return true;
  }

  if (match != (uint32_t) (-1)) {
    // Internal error.
    return false;
  }
  ctx->s1_struct_matches.pointer[s1_index] = s0_index;

  SVFRT_RangeStructDefinition structs0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r0, ctx->s0->structs, SVF_META_StructDefinition);
  SVFRT_RangeStructDefinition structs1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r1, ctx->s1->structs, SVF_META_StructDefinition);
  if (!structs0.pointer || !structs1.pointer) {
    // Internal error.
    return false;
  }

  SVF_META_StructDefinition *s0 = structs0.pointer + s0_index;
  SVF_META_StructDefinition *s1 = structs1.pointer + s1_index;

  SVFRT_RangeFieldDefinition fields0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r0, s0->fields, SVF_META_FieldDefinition);
  SVFRT_RangeFieldDefinition fields1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r1, s1->fields, SVF_META_FieldDefinition);

  if (!fields0.pointer || !fields0.pointer) {
    // Internal error.
    return false;
  }

  // Logical compatibility: all Schema 1 fields must be present in Schema 0,
  // with the same `name_hash`. Their types must be logically compatible.

  // Binary compatibility: all Schema 1 fields must be present in Schema 0, with
  // the same `name_hash` and `offset`. Their types must be binary compatible.

  // Exact compatibility: all Schema 1 fields must be present in Schema 0, with
  // the same `name_hash` and `offset`. Their types must be exactly compatible.
  // The sizes of structs must be equal.

  if (fields0.count < fields1.count) {
    // Early exit, because we know that at least one field from Schema 1 is not
    // present in Schema 0.
    return false;
  }

  // TODO @performance: N^2
  for (uint32_t i = 0; i < fields1.count; i++) {
    SVF_META_FieldDefinition *field1 = fields1.pointer + i;

    bool found = false;
    for (uint32_t j = 0; j < fields0.count; j++) {
      SVF_META_FieldDefinition *field0 = fields0.pointer + j;

      if (field0->name_hash == field1->name_hash) {
        if (
          (ctx->current_level >= SVFRT_compatibility_binary)
          && (field0->offset != field1->offset)
        ) {
          ctx->current_level = SVFRT_compatibility_logical;
          if (ctx->current_level < ctx->required_level) {
            return false;
          }
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

        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  if (ctx->current_level == SVFRT_compatibility_exact) {
    if (s0->size != s1->size) {
      ctx->current_level = SVFRT_compatibility_binary;
      if (ctx->current_level < ctx->required_level) {
        return false;
      }
    }
  }

  ctx->s1_struct_strides.pointer[s1_index] = s0->size;

  return true;
}

bool SVFRT_check_choice(
  SVFRT_CheckContext *ctx,
  uint32_t s0_index,
  uint32_t s1_index
) {
  uint32_t match = ctx->s1_choice_matches.pointer[s1_index];
  if (match == s0_index) {
    return true;
  }

  if (match != (uint32_t) (-1)) {
    // Internal error.
    return false;
  }
  ctx->s1_choice_matches.pointer[s1_index] = s0_index;

  SVFRT_RangeChoiceDefinition choices0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r0, ctx->s0->choices, SVF_META_ChoiceDefinition);
  SVFRT_RangeChoiceDefinition choices1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r1, ctx->s1->choices, SVF_META_ChoiceDefinition);
  if (!choices0.pointer || !choices1.pointer) {
    // Internal error.
    return false;
  }

  SVF_META_ChoiceDefinition *c0 = choices0.pointer + s0_index;
  SVF_META_ChoiceDefinition *c1 = choices1.pointer + s1_index;

  SVFRT_RangeOptionDefinition options0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r0, c0->options, SVF_META_OptionDefinition);
  SVFRT_RangeOptionDefinition options1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r1, c1->options, SVF_META_OptionDefinition);

  if (!options0.pointer || !options0.pointer) {
    // Internal error.
    return false;
  }

  // Logical compatibility: all Schema 0 fields must be present in Schema 1,
  // with the same `name_hash`. Their types must be logically compatible.

  // Binary compatibility: all Schema 0 options must be present in Schema 1, with
  // the same `name_hash` and `index`. Their types must be binary compatible.

  // Exact compatibility: all Schema 0 options must be present in Schema 1, with
  // the same `name_hash` and `index`. Their types must be exactly compatible.

  if (options1.count < options0.count) {
    // Early exit, because we know that at least one option from Schema 0 is not
    // present in Schema 1.
    return false;
  }

  // TODO @performance: N^2
  for (uint32_t i = 0; i < options0.count; i++) {
    SVF_META_OptionDefinition *option0 = options0.pointer + i;

    bool found = false;
    for (uint32_t j = 0; j < options1.count; j++) {
      SVF_META_OptionDefinition *option1 = options1.pointer + j;

      if (option0->name_hash == option1->name_hash) {
        if (
          (ctx->current_level >= SVFRT_compatibility_binary)
          && (option0->index != option1->index)
        ) {
          ctx->current_level = SVFRT_compatibility_logical;
          if (ctx->current_level < ctx->required_level) {
            return false;
          }
        }

        if (!check_type(
            ctx,
            option0->type_enum,
            option1->type_enum,
            &option0->type_union,
            &option1->type_union
          )) {
          return false;
        }

        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

void SVFRT_check_compatibility(
  SVFRT_CompatibilityResult *out_result,
  SVFRT_Bytes scratch_memory,
  SVFRT_Bytes schema_write,
  SVFRT_Bytes schema_read,
  uint64_t entry_name_hash,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_CompatibilityLevel sufficient_level
) {
  if (required_level == SVFRT_compatibility_none) {
    return;
  }

  if (sufficient_level < required_level) {
    return;
  }

  SVFRT_Bytes r0 = schema_write;
  SVFRT_Bytes r1 = schema_read;

  // TODO @proper-alignment.
  SVF_META_Schema *s0 = (SVF_META_Schema *) (r0.pointer + r0.count - sizeof(SVF_META_Schema));
  SVF_META_Schema *s1 = (SVF_META_Schema *) (r1.pointer + r1.count - sizeof(SVF_META_Schema));

  size_t partitions[3] = {
    sizeof(uint32_t) * s1->structs.count,
    sizeof(uint32_t) * s1->choices.count,
    sizeof(uint32_t) * s1->structs.count,
  };
  size_t total_needed = (
    partitions[0]
    + partitions[1]
    + partitions[2]
  );
  if (scratch_memory.count < total_needed) {
    return;
  }

  SVFRT_RangeU32 s1_struct_matches = {
    /*.pointer =*/ (uint32_t *) scratch_memory.pointer,
    /*.count =*/ s1->structs.count
  };
  SVFRT_RangeU32 s1_choice_matches = {
    /*.pointer =*/ (uint32_t *) (scratch_memory.pointer + partitions[0]),
    /*.count =*/ s1->choices.count
  };
  SVFRT_RangeU32 s1_struct_strides = {
    /*.pointer =*/ (uint32_t *) (scratch_memory.pointer + partitions[0] + partitions[1]),
    /*.count =*/ s1->structs.count
  };

  for (uint32_t i = 0; i < s1->structs.count; i++) {
    s1_struct_matches.pointer[i] = (uint32_t) (-1);
    s1_struct_strides.pointer[i] = 0;
  }
  for (uint32_t i = 0; i < s1->choices.count; i++) {
    s1_choice_matches.pointer[i] = (uint32_t) (-1);
  }

  SVFRT_CheckContext ctx_val = {
    .s0 = s0,
    .s1 = s1,
    .r0 = r0,
    .r1 = r1,
    .s1_struct_matches = s1_struct_matches,
    .s1_choice_matches = s1_choice_matches,
    .s1_struct_strides = s1_struct_strides,
    .current_level = sufficient_level,
    .required_level = required_level
  };
  SVFRT_CheckContext *ctx = &ctx_val;

  uint32_t struct_index0 = (uint32_t) (-1);
  uint32_t struct_index1 = (uint32_t) (-1);

  SVFRT_RangeStructDefinition structs0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r0, ctx->s0->structs, SVF_META_StructDefinition);
  SVFRT_RangeStructDefinition structs1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->r1, ctx->s1->structs, SVF_META_StructDefinition);

  if (!structs0.pointer || !structs1.pointer) {
    // Internal error.
    return;
  }

  for (uint32_t i = 0; i < structs0.count; i++) {
    if (structs0.pointer[i].name_hash == entry_name_hash) {
      struct_index0 = i;
      break;
    }
  }

  for (uint32_t i = 0; i < structs1.count; i++) {
    if (structs1.pointer[i].name_hash == entry_name_hash) {
      struct_index1 = i;
      break;
    }
  }

  if (struct_index0 == (uint32_t) (-1) || struct_index1 == (uint32_t) (-1)) {
    // Internal error.
    return;
  }

  if (!SVFRT_check_struct(
    ctx,
    struct_index0,
    struct_index1
  )) {
    return;
  }

  if (ctx->current_level == SVFRT_compatibility_logical) {
    // If we have logical compatibility, then we will do a conversion, so it
    // does not make sense to store Schema 0 sizes here.
    for (size_t i = 0; i < structs1.count; i++) {
      ctx->s1_struct_strides.pointer[i] = structs1.pointer[i].size;
    }
  }

  out_result->level = ctx->current_level;
  out_result->struct_strides = ctx->s1_struct_strides;
  out_result->entry_size0 = structs0.pointer[struct_index0].size;
  out_result->entry_struct_index0 = struct_index0;
  out_result->entry_struct_index1 = struct_index1;
}
