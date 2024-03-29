#ifndef SVFRT_SINGLE_FILE
  #include "svf_runtime.h"
  #include "svf_internal.h"
#endif

// Note: #unsafe-naming-semantics.
//
// We are reading potentially adversarial data, and it's important to delineate,
// which is which. "Unsafe" value in this context means that is is accessible,
// but may contain something completely unexpected or malicious. This concerns:
//
// - Value types, e.g. `unsafe_index` would mean that the index itself may be
//   out of bounds.
//
// - Range types, e.g. `unsafe_xs` would mean that a contiguous memory area
//   with X-s is accessible, but the X-s themselves can be anything.
//
// - Pointer types, which can be dereferenced, but the value cannot be relied upon.
//
// For all pointers, nullability is a separate concern and is handled independedntly
// of this.

typedef struct SVFRT_IndexPair {
  uint32_t index_src;
  uint32_t index_dst;
} SVFRT_IndexPair;
// TODO: sizeof must be 8, and alignof must be 4. Check this at compile time.

typedef struct SVFRT_IndexPairQueue {
  SVFRT_IndexPair *pointer;
  uint32_t capacity;
  uint32_t occupied;
} SVFRT_IndexPairQueue;

typedef struct SVFRT_CheckContext {
  // The root definition for both schemas.
  SVF_Meta_SchemaDefinition *unsafe_definition_src;
  SVF_Meta_SchemaDefinition *definition_dst;

  // Byte ranges of the two schemas.
  SVFRT_Bytes unsafe_schema_src;
  SVFRT_Bytes schema_dst;

  SVFRT_RangeStructDefinition unsafe_structs_src;
  SVFRT_RangeStructDefinition structs_dst;

  SVFRT_RangeChoiceDefinition unsafe_choices_src;
  SVFRT_RangeChoiceDefinition choices_dst;

  // What do each of dst-schema structs/choices match to in src-schema?
  SVFRT_RangeU32 dst_to_src_struct_matches;
  SVFRT_RangeU32 dst_to_src_choice_matches;

  // What stride should be used when indexing a sequence of structs? With binary
  // compatibility, it is not size of the dst-schema struct, even though we are
  // accessing the data through it. It is actually the size of the src-schema
  // struct, since that's what the original sequence was using as the stride.
  SVFRT_RangeU32 unsafe_struct_strides_dst;

  // Fixed-size FIFO queues in scratch memory to avoid using the real stack, and
  // avoid any potential overflow problems.
  SVFRT_IndexPairQueue struct_queue;
  SVFRT_IndexPairQueue choice_queue;

  SVFRT_RangeU32 field_matches_header;
  SVFRT_RangeU32 field_matches;

  SVFRT_RangeU32 option_matches_header;
  SVFRT_Bytes option_matches_tags;
  SVFRT_RangeU32 option_matches;

  // Lowest level seen so far. Should be >= required level.
  SVFRT_CompatibilityLevel current_level;

  // Required level. We can exit early if we see that something does not match it.
  SVFRT_CompatibilityLevel required_level;

  uint32_t max_schema_work;
  uint32_t work_done;

  SVFRT_ErrorCode error_code; // See `SVFRT_code_compatibility__*`.
} SVFRT_CheckContext;

void SVFRT_check_add_struct(
  SVFRT_CheckContext *ctx,
  uint32_t struct_index_src,
  uint32_t struct_index_dst
) {
  uint32_t match = ctx->dst_to_src_struct_matches.pointer[struct_index_dst];
  if (match == struct_index_src) {
    // We previously checked how this dst-index matches this src-index, and it
    // was fine.
    return;
  }

  if (match != UINT32_MAX) {
    // We previously matched this dst-index to a different src-index, so
    // something is wrong.
    ctx->error_code = SVFRT_code_compatibility__struct_index_mismatch;
    return;
  }

  ctx->dst_to_src_struct_matches.pointer[struct_index_dst] = struct_index_src;

  if (ctx->struct_queue.occupied >= ctx->struct_queue.capacity) {
    ctx->error_code = SVFRT_code_compatibility_internal__queue_overflow;
    return;
  }
  SVFRT_IndexPair *pair = ctx->struct_queue.pointer + ctx->struct_queue.occupied;
  ctx->struct_queue.occupied++;

  pair->index_src = struct_index_src;
  pair->index_dst = struct_index_dst;
}

void SVFRT_check_add_choice(
  SVFRT_CheckContext *ctx,
  uint32_t choice_index_src,
  uint32_t choice_index_dst
) {
  uint32_t match = ctx->dst_to_src_choice_matches.pointer[choice_index_dst];
  if (match == choice_index_src) {
    // We previously checked how this dst-index matches this src-index, and it
    // was fine.
    return;
  }

  if (match != UINT32_MAX) {
    // We previously matched this dst-index to a different src-index, so
    // something is wrong.
    ctx->error_code = SVFRT_code_compatibility__choice_index_mismatch;
    return;
  }

  ctx->dst_to_src_choice_matches.pointer[choice_index_dst] = choice_index_src;

  if (ctx->choice_queue.occupied >= ctx->choice_queue.capacity) {
    ctx->error_code = SVFRT_code_compatibility_internal__queue_overflow;
    return;
  }
  SVFRT_IndexPair *pair = ctx->choice_queue.pointer + ctx->choice_queue.occupied;
  ctx->choice_queue.occupied++;

  pair->index_src = choice_index_src;
  pair->index_dst = choice_index_dst;
}

static inline
bool SVFRT_check_work(SVFRT_CheckContext *ctx, uint32_t work_count) {
  ctx->work_done += work_count;
  if (ctx->work_done > ctx->max_schema_work) {
    ctx->error_code = SVFRT_code_compatibility__max_schema_work_exceeded;
    return false;
  }

  return true;
}

// TODO: report the specific types that did not match?
void SVFRT_check_concrete_type(
  SVFRT_CheckContext *ctx,
  SVF_Meta_ConcreteType_tag unsafe_tag_src,
  SVF_Meta_ConcreteType_payload *unsafe_payload_src,
  SVF_Meta_ConcreteType_tag tag_dst,
  SVF_Meta_ConcreteType_payload *payload_dst
) {
  if (unsafe_tag_src == tag_dst) {
    switch (tag_dst) {
      case SVF_Meta_ConcreteType_tag_definedStruct: {
        uint32_t unsafe_src_index = unsafe_payload_src->definedStruct.index;

        // This guarantees that the index is valid, and is also not `UINT32_MAX`.
        if (unsafe_src_index >= ctx->unsafe_structs_src.count) {
          ctx->error_code = SVFRT_code_compatibility__invalid_struct_index;
          return;
        }

        SVFRT_check_add_struct(
          ctx,
          unsafe_src_index,
          payload_dst->definedStruct.index
        );
        return;
      }
      case SVF_Meta_ConcreteType_tag_definedChoice: {
        uint32_t unsafe_src_index = unsafe_payload_src->definedChoice.index;

        // This guarantees that the index is valid, and is also not `UINT32_MAX`.
        if (unsafe_src_index >= ctx->unsafe_choices_src.count) {
          ctx->error_code = SVFRT_code_compatibility__invalid_choice_index;
          return;
        }

        SVFRT_check_add_choice(
          ctx,
          unsafe_src_index,
          payload_dst->definedChoice.index
        );
        return;
      }
      // TODO: this is not exhaustive, but there's not much to gain from that?
      default: {
        // Other types are primitive, so tag equality is enough.
        return;
      }
    }
  }

  if (ctx->required_level > SVFRT_compatibility_logical) {
    ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
    return;
  }

  switch (tag_dst) {
    case SVF_Meta_ConcreteType_tag_u16: {
      if (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u8) {
        ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
      }
      return;
    }
    case SVF_Meta_ConcreteType_tag_u32: {
      if (
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u16) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u8)
      ) {
        ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
      }
      return;
    }
    case SVF_Meta_ConcreteType_tag_u64: {
      if (
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u32) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u16) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u8)
      ) {
        ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
      }
      return;
    }
    case SVF_Meta_ConcreteType_tag_i16: {
      if (
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_i8) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u8)
      ) {
        ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
      }
      return;
    }
    case SVF_Meta_ConcreteType_tag_i32: {
      if (
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_i16) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_i8) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u16) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u8)
      ) {
        ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
      }
      return;
    }
    case SVF_Meta_ConcreteType_tag_i64: {
      if (
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_i32) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_i16) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_i8) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u32) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u16) &&
        (unsafe_tag_src != SVF_Meta_ConcreteType_tag_u8)
      ) {
        ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
      }
      return;
    }
    case SVF_Meta_ConcreteType_tag_f64: {
      if (unsafe_tag_src != SVF_Meta_ConcreteType_tag_f32) {
        ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
      }
      return;
    }
    // TODO: this is not exhaustive, but there's not much to gain from that?
    default: {
      ctx->error_code = SVFRT_code_compatibility__concrete_type_mismatch;
      return;
    }
  }
}

void SVFRT_check_type(
  SVFRT_CheckContext *ctx,
  SVF_Meta_Type_tag unsafe_tag_src,
  SVF_Meta_Type_payload *unsafe_payload_src,
  SVF_Meta_Type_tag tag_dst,
  SVF_Meta_Type_payload *payload_dst
) {
  if (unsafe_tag_src != tag_dst) {
    ctx->error_code = SVFRT_code_compatibility__type_mismatch;
    return;
  }

  switch (tag_dst) {
    case SVF_Meta_Type_tag_reference: {
      SVFRT_check_concrete_type(
        ctx,
        unsafe_payload_src->reference.type_tag,
        &unsafe_payload_src->reference.type_payload,
        payload_dst->reference.type_tag,
        &payload_dst->reference.type_payload
      );
      return;
    }
    case SVF_Meta_Type_tag_sequence: {
      SVFRT_check_concrete_type(
        ctx,
        unsafe_payload_src->sequence.elementType_tag,
        &unsafe_payload_src->sequence.elementType_payload,
        payload_dst->sequence.elementType_tag,
        &payload_dst->sequence.elementType_payload
      );
      return;
    }
    case SVF_Meta_Type_tag_concrete: {
      SVFRT_check_concrete_type(
        ctx,
        unsafe_payload_src->concrete.type_tag,
        &unsafe_payload_src->concrete.type_payload,
        payload_dst->concrete.type_tag,
        &payload_dst->concrete.type_payload
      );
      return;
    }
    default: {
      ctx->error_code = SVFRT_code_compatibility_internal__invalid_type_tag;
      return;
    }
  }
}

void SVFRT_check_struct(
  SVFRT_CheckContext *ctx,
  uint32_t struct_index_src,
  uint32_t struct_index_dst
) {
  // Field-matches header should have valid indices for every dst-struct, so no
  // range checking is required here.
  uint32_t field_matches_index = ctx->field_matches_header.pointer[struct_index_dst];

  SVF_Meta_StructDefinition *unsafe_definition_src = ctx->unsafe_structs_src.pointer + struct_index_src;
  SVF_Meta_StructDefinition *definition_dst = ctx->structs_dst.pointer + struct_index_dst;

  // Safety: out-of-bounds access will be caught, and `.pointer` will be NULL.
  SVFRT_RangeFieldDefinition unsafe_fields_src = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    ctx->unsafe_schema_src,
    unsafe_definition_src->fields,
    SVF_Meta_FieldDefinition
  );
  if (!unsafe_fields_src.pointer && unsafe_fields_src.count) {
    ctx->error_code = SVFRT_code_compatibility__invalid_fields;
    return;
  }

  // Safety: out-of-bounds access will be caught, and `.pointer` will be NULL.
  SVFRT_RangeFieldDefinition fields_dst = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    ctx->schema_dst,
    definition_dst->fields,
    SVF_Meta_FieldDefinition
  );
  if (!fields_dst.pointer && fields_dst.count) {
    ctx->error_code = SVFRT_code_compatibility_internal__invalid_fields;
    return;
  }

  // Compatibility rules for structs:
  // ================================
  // TODO: Additional review for these rules. They look right, but there might be
  // quirks.
  //
  // - All positive-polarity DST-fields must match one of the SRC-fields by ID.
  // - All negative-polarity SRC-fields must match one of the DST-fields by ID.
  //
  // (This implies that positive-polarity SRC-fields might not have matches,
  // which essentially means they may be ignored.)
  //
  // (And also, negative-polarity DST-fields might not have matches,
  // which essentially means they may be created and zero-initialized.)
  //
  // Logical compatibility:
  // - All field ID matches:
  //   - Both have the same polarity flag.
  //   - Their types are logically compatible.
  //   - Either:
  //     - Both are marked as present (not removed).
  //     - Both are marked as removed.
  //     - Both have positive polarity, the SRC-field is removed, and the DST-field is present.
  //     - Both have negative polarity, the DST-field is removed, and the SRC-field is present.
  //
  // Binary compatibility:
  // - All DST-fields must match one of the SRC-fields by ID.
  // - All field ID matches must:
  //   - Have the same ID, offset, polarity flag, and removal flag.
  //   - Their types be binary compatible.
  // - The size of the DST-struct must be _less or equal_ to the size of the SRC-struct.
  //
  // Exact compatibility:
  // - All DST-fields must match one of the SRC-fields by ID.
  // - All field ID matches must:
  //   - Have the same ID, offset, polarity flag, and removal flag.
  //   - Their types be exactly compatible.
  // - The size of the DST-struct must be _EQUAL_ to the size of the SRC-struct.

  // Looping over potentially adversarial data, so limit work. #compatibility-work-fields.
  if (!SVFRT_check_work(ctx, 2 * unsafe_fields_src.count * fields_dst.count)) {
    return;
  }

  for (uint32_t i = 0; i < fields_dst.count; i++) {
    SVF_Meta_FieldDefinition *field_dst = fields_dst.pointer + i;
    bool inverted_polarity_dst = (field_dst->fieldId & (1ull << 63)) != 0;

    // Field-matches table should be valid for every possible dst-field, so no
    // range checking is required here.
    uint32_t *out_match = ctx->field_matches.pointer + field_matches_index + i;

    bool found = false;
    for (uint32_t j = 0; j < unsafe_fields_src.count; j++) {
      SVF_Meta_FieldDefinition *unsafe_field_src = unsafe_fields_src.pointer + j;

      if (unsafe_field_src->fieldId == field_dst->fieldId) {
        if (!inverted_polarity_dst && !unsafe_field_src->removed && field_dst->removed) {
          // TODO: not covered by tests yet.
          ctx->error_code = SVFRT_code_compatibility__field_cannot_be_ignored;
          return;
        }

        if (inverted_polarity_dst && unsafe_field_src->removed && !field_dst->removed) {
          ctx->error_code = SVFRT_code_compatibility__field_is_missing;
          return;
        }

        if (!unsafe_field_src->removed && !field_dst->removed) {
          SVFRT_check_type(
            ctx,
            unsafe_field_src->type_tag,
            &unsafe_field_src->type_payload,
            field_dst->type_tag,
            &field_dst->type_payload
          );

          if (ctx->error_code) {
            return;
          }
        }

        if (ctx->current_level >= SVFRT_compatibility_binary) {
          // Additional checks for binary compatibility, that either downgrade
          // the current level to logical compatibility, or return an error.

          if (unsafe_field_src->offset != field_dst->offset) {
            ctx->current_level = SVFRT_compatibility_logical;
            if (ctx->current_level < ctx->required_level) {
              ctx->error_code = SVFRT_code_compatibility__field_offset_mismatch;
              return;
            }
          }

          if (!unsafe_field_src->removed && field_dst->removed) {
            ctx->current_level = SVFRT_compatibility_logical;
            if (ctx->current_level < ctx->required_level) {
              ctx->error_code = SVFRT_code_compatibility__field_cannot_be_ignored;
              return;
            }
          };

          if (inverted_polarity_dst && unsafe_field_src->removed && !field_dst->removed) {
            ctx->current_level = SVFRT_compatibility_logical;
            if (ctx->current_level < ctx->required_level) {
              ctx->error_code = SVFRT_code_compatibility__field_is_missing;
              return;
            }
          }
        }

        // Note: an incorrect src-schema could have multiple fields with the
        // same ID. We will simply ignore those after the first one here.
        //
        // This is equivalent to having additional src-fields that do not match
        // to anything, and should not cause any problems.

        if (!unsafe_field_src->removed) {
          *out_match = j;
        }
        found = true;
        break;
      }
    }

    if (!found) {
      if (!inverted_polarity_dst) {
        ctx->error_code = SVFRT_code_compatibility__field_is_missing;
        return;
      }

      // For binary compatibility, missing negative-polarity src-fields are also an
      // error.
      if (ctx->current_level >= SVFRT_compatibility_binary) {
        ctx->current_level = SVFRT_compatibility_logical;
        if (ctx->current_level < ctx->required_level) {
          ctx->error_code = SVFRT_code_compatibility__field_is_missing;
          return;
        }
      }
    }
  }

  for (uint32_t i = 0; i < unsafe_fields_src.count; i++) {
    SVF_Meta_FieldDefinition *unsafe_field_src = unsafe_fields_src.pointer + i;
    bool inverted_polarity_src = (unsafe_field_src->fieldId & (1ull << 63)) != 0;

    bool found = false;
    for (uint32_t j = 0; j < fields_dst.count; j++) {
      SVF_Meta_FieldDefinition *field_dst = fields_dst.pointer + j;

      if (unsafe_field_src->fieldId == field_dst->fieldId) {
        found = true;
        break;
      }
    }

    if (!found) {
      if (inverted_polarity_src) {
        // TODO: not covered by tests yet.
        ctx->error_code = SVFRT_code_compatibility__field_cannot_be_ignored;
        return;
      }
    }
  }

  if (ctx->current_level == SVFRT_compatibility_exact) {
    // #dst-stride-is-sane.
    if (unsafe_definition_src->size != definition_dst->size) {
      // Sizes do not match, downgrade to binary compatibility.
      ctx->current_level = SVFRT_compatibility_binary;
      if (ctx->current_level < ctx->required_level) {
        ctx->error_code = SVFRT_code_compatibility__struct_size_mismatch;
        return;
      }
    }
  }

  if (ctx->current_level == SVFRT_compatibility_binary) {
    // #dst-stride-is-sane.
    if (unsafe_definition_src->size < definition_dst->size) {
      // Can't provide binary compatibility, so downgrade to logical compatibility.
      ctx->current_level = SVFRT_compatibility_logical;
      if (ctx->current_level < ctx->required_level) {
        ctx->error_code = SVFRT_code_compatibility__struct_size_mismatch;
        return;
      }
    }
  }

  ctx->unsafe_struct_strides_dst.pointer[struct_index_dst] = unsafe_definition_src->size;
}

void SVFRT_check_choice(
  SVFRT_CheckContext *ctx,
  uint32_t choice_index_src,
  uint32_t choice_index_dst
) {
  // Option matches header should have valid indices for every dst-choice, so
  // no range checking is required here.
  uint32_t option_matches_index = ctx->option_matches_header.pointer[choice_index_dst];

  SVF_Meta_ChoiceDefinition *unsafe_definition_src = ctx->unsafe_choices_src.pointer + choice_index_src;
  SVF_Meta_ChoiceDefinition *definition_dst = ctx->choices_dst.pointer + choice_index_dst;

  // Safety: out-of-bounds access will be caught, and `.pointer` will be NULL.
  SVFRT_RangeOptionDefinition unsafe_options_src = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    ctx->unsafe_schema_src,
    unsafe_definition_src->options,
    SVF_Meta_OptionDefinition
  );
  if (!unsafe_options_src.pointer && unsafe_options_src.count) {
    ctx->error_code = SVFRT_code_compatibility__invalid_options;
    return;
  }

  // Safety: out-of-bounds access will be caught, and `.pointer` will be NULL.
  SVFRT_RangeOptionDefinition options_dst = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    ctx->schema_dst,
    definition_dst->options,
    SVF_Meta_OptionDefinition
  );
  if (!options_dst.pointer && options_dst.count) {
    ctx->error_code = SVFRT_code_compatibility_internal__invalid_options;
    return;
  }

  // Compatibility rules for choices:
  // ================================
  // TODO: Additional review for these rules. Note: they are mostly adapted from
  // struct rules and there might be quirks.
  //
  // - All positive-polarity DST-options must match one of the SRC-options by ID.
  // - All negative-polarity SRC-options must match one of the DST-options by ID.
  //
  // (This implies that positive-polarity SRC-options might not have matches,
  // which essentially means they will be reset to tag 0.)
  //
  // (And also, negative-polarity DST-options might not have matches,
  // which essentially means they will not be encountered.)
  //
  // Logical compatibility:
  // - All option ID matches:
  //   - Both have the same polarity flag.
  //   - Their types are logically compatible.
  //   - Either:
  //     - Both are marked as present (not removed).
  //     - Both are marked as removed.
  //     - Both have positive polarity, the SRC-option is removed, and the DST-option is present.
  //     - Both have negative polarity, the DST-option is removed, and the SRC-option is present.
  //
  // Binary compatibility:
  // - All option ID matches must:
  //   - Have the same ID, offset, polarity flag, and removal flag.
  //   - Their types be binary compatible.
  //
  // Exact compatibility:
  // - All option ID matches must:
  //   - Have the same ID, offset, polarity flag, and removal flag.
  //   - Their types be exactly compatible.

  // Looping over potentially adversarial data, so limit work. #compatibility-work-options
  if (!SVFRT_check_work(ctx, 2 * unsafe_options_src.count * options_dst.count)) {
    return;
  }
  for (uint32_t i = 0; i < options_dst.count; i++) {
    SVF_Meta_OptionDefinition *option_dst = options_dst.pointer + i;
    bool inverted_polarity_dst = (option_dst->optionId & (1ull << 63)) != 0;

    // Option-matches table should be valid for every possible dst-option, so no
    // range checking is required here.
    uint32_t *out_match = ctx->option_matches.pointer + option_matches_index + i;
    uint8_t *out_tag = ctx->option_matches_tags.pointer + option_matches_index + i;

    bool found = false;
    for (uint32_t j = 0; j < unsafe_options_src.count; j++) {
      SVF_Meta_OptionDefinition *unsafe_option_src = unsafe_options_src.pointer + j;

      if (unsafe_option_src->optionId == option_dst->optionId) {
        if (!inverted_polarity_dst && !unsafe_option_src->removed && option_dst->removed) {
          // TODO: not covered by tests yet.
          ctx->error_code = SVFRT_code_compatibility__option_cannot_be_ignored;
          return;
        }

        if (inverted_polarity_dst && unsafe_option_src->removed && !option_dst->removed) {
          ctx->error_code = SVFRT_code_compatibility__option_is_missing;
          return;
        }

        if (!unsafe_option_src->removed && !option_dst->removed) {
          SVFRT_check_type(
            ctx,
            unsafe_option_src->type_tag,
            &unsafe_option_src->type_payload,
            option_dst->type_tag,
            &option_dst->type_payload
          );

          if (ctx->error_code) {
            return;
          }
        }

        if (ctx->current_level >= SVFRT_compatibility_binary) {
          // Additional checks for binary compatibility, that either downgrade
          // the current level to logical compatibility, or return an error.

          if (unsafe_option_src->tag != option_dst->tag) {
            ctx->current_level = SVFRT_compatibility_logical;
            if (ctx->current_level < ctx->required_level) {
              ctx->error_code = SVFRT_code_compatibility__option_tag_mismatch;
              return;
            }
          }

          if (!unsafe_option_src->removed && option_dst->removed) {
            ctx->current_level = SVFRT_compatibility_logical;
            if (ctx->current_level < ctx->required_level) {
              ctx->error_code = SVFRT_code_compatibility__option_cannot_be_ignored;
              return;
            }
          };

          if (inverted_polarity_dst && unsafe_option_src->removed && !option_dst->removed) {
            ctx->current_level = SVFRT_compatibility_logical;
            if (ctx->current_level < ctx->required_level) {
              ctx->error_code = SVFRT_code_compatibility__option_is_missing;
              return;
            }
          }
        }

        // Note: an incorrect src-schema could have multiple options with the
        // same ID. We will simply ignore those after the first one here.
        //
        // This is equivalent to having additional src-options that do not match
        // to anything, and should not cause any problems.

        if (!unsafe_option_src->removed) {
          if (unsafe_option_src->tag == 0) {
            ctx->error_code = SVFRT_code_compatibility__invalid_tag;
          }

          *out_tag = unsafe_option_src->tag;
          *out_match = j;
        }
        found = true;
        break;
      }
    }

    if (!found) {
      if (!inverted_polarity_dst) {
        ctx->error_code = SVFRT_code_compatibility__option_is_missing;
        return;
      }
    }
  }

  for (uint32_t i = 0; i < unsafe_options_src.count; i++) {
    SVF_Meta_OptionDefinition *unsafe_option_src = unsafe_options_src.pointer + i;
    bool inverted_polarity_src = (unsafe_option_src->optionId & (1ull << 63)) != 0;

    bool found = false;
    for (uint32_t j = 0; j < options_dst.count; j++) {
      SVF_Meta_OptionDefinition *option_dst = options_dst.pointer + j;

      if (unsafe_option_src->optionId == option_dst->optionId) {
        found = true;
        break;
      }
    }

    if (!found) {
      if (inverted_polarity_src) {
        // TODO: not covered by tests yet.
        ctx->error_code = SVFRT_code_compatibility__option_cannot_be_ignored;
        return;
      }
    }
  }
}

void SVFRT_check_compatibility(
  SVFRT_CompatibilityResult *out_result,
  SVFRT_Bytes scratch_memory,
  SVFRT_Bytes unsafe_schema_src,
  SVFRT_Bytes schema_dst,
  uint64_t entry_struct_id,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_CompatibilityLevel sufficient_level,
  uint32_t max_schema_work
) {
  if (required_level == SVFRT_compatibility_none) {
    out_result->error_code = SVFRT_code_compatibility__required_level_is_none;
    return;
  }

  if (sufficient_level < required_level) {
    out_result->error_code = SVFRT_code_compatibility__invalid_sufficient_level;
    return;
  }

  // Make sure we can read both schemas.

  if (unsafe_schema_src.count < sizeof(SVF_Meta_SchemaDefinition)) {
    out_result->error_code = SVFRT_code_compatibility__schema_too_small;
    return;
  }
  // TODO @proper-alignment: struct access.
  SVF_Meta_SchemaDefinition *unsafe_definition_src = (SVF_Meta_SchemaDefinition *) (
    unsafe_schema_src.pointer
    + unsafe_schema_src.count
    - sizeof(SVF_Meta_SchemaDefinition)
  );

  if (schema_dst.count < sizeof(SVF_Meta_SchemaDefinition)) {
    out_result->error_code = SVFRT_code_compatibility_internal__schema_too_small;
    return;
  }
  // TODO @proper-alignment: struct access.
  SVF_Meta_SchemaDefinition *definition_dst = (SVF_Meta_SchemaDefinition *) (
    schema_dst.pointer
    + schema_dst.count
    - sizeof(SVF_Meta_SchemaDefinition)
  );

  // Safety: out-of-bounds access will be caught, and `.pointer` will be NULL.
  SVFRT_RangeStructDefinition unsafe_structs_src = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    unsafe_schema_src,
    unsafe_definition_src->structs,
    SVF_Meta_StructDefinition
  );
  if (!unsafe_structs_src.pointer && unsafe_structs_src.count) {
    out_result->error_code = SVFRT_code_compatibility__invalid_structs;
    return;
  }

  // Safety: out-of-bounds access will be caught, and `.pointer` will be NULL.
  SVFRT_RangeStructDefinition structs_dst = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    schema_dst,
    definition_dst->structs,
    SVF_Meta_StructDefinition
  );
  if (!structs_dst.pointer && structs_dst.count) {
    out_result->error_code = SVFRT_code_compatibility_internal__invalid_structs;
    return;
  }

  // Same for choices.
  // Safety: out-of-bounds access will be caught, and `.pointer` will be NULL.
  SVFRT_RangeChoiceDefinition unsafe_choices_src = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    unsafe_schema_src,
    unsafe_definition_src->choices,
    SVF_Meta_ChoiceDefinition
  );
  if (!unsafe_choices_src.pointer && unsafe_choices_src.count) {
    out_result->error_code = SVFRT_code_compatibility__invalid_choices;
    return;
  }

  // Safety: out-of-bounds access will be caught, and `.pointer` will be NULL.
  SVFRT_RangeChoiceDefinition choices_dst = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    schema_dst,
    definition_dst->choices,
    SVF_Meta_ChoiceDefinition
  );
  if (!choices_dst.pointer && choices_dst.count) {
    out_result->error_code = SVFRT_code_compatibility_internal__invalid_choices;
    return;
  }

  uint32_t field_matches_count = 0;
  for (uint32_t i = 0; i < structs_dst.count; i++) {
    SVF_Meta_StructDefinition *definition_dst = structs_dst.pointer + i;
    field_matches_count += definition_dst->fields.count;
  }

  uint32_t option_matches_count = 0;
  for (uint32_t i = 0; i < choices_dst.count; i++) {
    SVF_Meta_ChoiceDefinition *definition_dst = choices_dst.pointer + i;
    option_matches_count += definition_dst->options.count;
  }

  uint32_t scratch_misalignment = ((uintptr_t) scratch_memory.pointer) % sizeof(uint32_t);
  uint32_t scratch_padding = scratch_misalignment ? sizeof(uint32_t) - scratch_misalignment : 0;

  // See #scratch-memory-partitions.
  size_t partitions[10] = {
    sizeof(uint32_t) * definition_dst->structs.count, // Strides.
    sizeof(uint32_t) * definition_dst->structs.count, // Matches.
    sizeof(uint32_t) * definition_dst->choices.count, // Matches.
    sizeof(SVFRT_IndexPair) * definition_dst->structs.count, // Queue.
    sizeof(SVFRT_IndexPair) * definition_dst->choices.count,  // Queue.
    sizeof(uint32_t) * definition_dst->structs.count, // Field-matches header.
    sizeof(uint32_t) * field_matches_count, // Field-matches.
    sizeof(uint32_t) * definition_dst->choices.count, // Option-matches header.
    sizeof(uint32_t) * option_matches_count, // Option-matches.
    sizeof(uint8_t) * option_matches_count // Option-matches tags.
  };

  size_t total_scratch_needed = (
    scratch_padding
    + partitions[0]
    + partitions[1]
    + partitions[2]
    + partitions[3]
    + partitions[4]
    + partitions[5]
    + partitions[6]
    + partitions[7]
    + partitions[8]
    + partitions[9]
  );
  if (scratch_memory.count < total_scratch_needed) {
    out_result->error_code = SVFRT_code_compatibility__not_enough_scratch_memory;
    return;
  }

  uint8_t *partition_pointer = scratch_memory.pointer + scratch_padding;
  SVFRT_RangeU32 unsafe_struct_strides_dst = {
    /*.pointer =*/ (uint32_t *) partition_pointer,
    /*.count =*/ definition_dst->structs.count
  };

  partition_pointer += partitions[0];
  SVFRT_RangeU32 dst_to_src_struct_matches = {
    /*.pointer =*/ (uint32_t *) partition_pointer,
    /*.count =*/ definition_dst->structs.count
  };

  partition_pointer += partitions[1];
  SVFRT_RangeU32 dst_to_src_choice_matches = {
    /*.pointer =*/ (uint32_t *) partition_pointer,
    /*.count =*/ definition_dst->choices.count
  };

  partition_pointer += partitions[2];
  SVFRT_IndexPairQueue struct_queue_initial = {
    /*.pointer =*/ (SVFRT_IndexPair *) partition_pointer,
    /*.capacity =*/ definition_dst->structs.count,
    /*.occupied =*/ 0
  };

  partition_pointer += partitions[3];
  SVFRT_IndexPairQueue choice_queue_initial = {
    /*.pointer =*/ (SVFRT_IndexPair *) partition_pointer,
    /*.capacity =*/ definition_dst->choices.count,
    /*.occupied =*/ 0
  };

  partition_pointer += partitions[4];
  SVFRT_RangeU32 field_matches_header = {
    /*.pointer =*/ (uint32_t *) partition_pointer,
    /*.count =*/ definition_dst->structs.count
  };

  partition_pointer += partitions[5];
  SVFRT_RangeU32 field_matches = {
    /*.pointer =*/ (uint32_t *) partition_pointer,
    /*.count =*/ field_matches_count
  };

  partition_pointer += partitions[6];
  SVFRT_RangeU32 option_matches_header = {
    /*.pointer =*/ (uint32_t *) partition_pointer,
    /*.count =*/ definition_dst->choices.count
  };

  partition_pointer += partitions[7];
  SVFRT_RangeU32 option_matches = {
    /*.pointer =*/ (uint32_t *) partition_pointer,
    /*.count =*/ option_matches_count
  };

  partition_pointer += partitions[8];
  SVFRT_Bytes option_matches_tags = {
    /*.pointer =*/ (uint8_t *) partition_pointer,
    /*.count =*/ option_matches_count
  };

  for (uint32_t i = 0; i < unsafe_struct_strides_dst.count; i++) {
    unsafe_struct_strides_dst.pointer[i] = 0;
  }
  for (uint32_t i = 0; i < dst_to_src_struct_matches.count; i++) {
    dst_to_src_struct_matches.pointer[i] = UINT32_MAX;
  }
  for (uint32_t i = 0; i < dst_to_src_choice_matches.count; i++) {
    dst_to_src_choice_matches.pointer[i] = UINT32_MAX;
  }
  for (uint32_t i = 0; i < struct_queue_initial.capacity; i++) {
    struct_queue_initial.pointer[i].index_src = UINT32_MAX;
    struct_queue_initial.pointer[i].index_dst = UINT32_MAX;
  }
  for (uint32_t i = 0; i < choice_queue_initial.capacity; i++) {
    choice_queue_initial.pointer[i].index_src = UINT32_MAX;
    choice_queue_initial.pointer[i].index_dst = UINT32_MAX;
  }
  uint32_t total_fields = 0;
  for (uint32_t i = 0; i < structs_dst.count; i++) {
    SVF_Meta_StructDefinition *definition_dst = structs_dst.pointer + i;
    field_matches_header.pointer[i] = total_fields;
    total_fields += definition_dst->fields.count;
  }
  for (uint32_t i = 0; i < field_matches.count; i++) {
    field_matches.pointer[i] = UINT32_MAX;
  }
  uint32_t total_options = 0;
  for (uint32_t i = 0; i < choices_dst.count; i++) {
    SVF_Meta_ChoiceDefinition *definition_dst = choices_dst.pointer + i;
    option_matches_header.pointer[i] = total_options;
    total_options += definition_dst->options.count;
  }
  for (uint32_t i = 0; i < option_matches.count; i++) {
    option_matches.pointer[i] = UINT32_MAX;
    option_matches_tags.pointer[i] = 0;
  }

  SVFRT_CheckContext ctx_val = {
    .unsafe_definition_src = unsafe_definition_src,
    .definition_dst = definition_dst,
    .unsafe_schema_src = unsafe_schema_src,
    .schema_dst = schema_dst,
    .unsafe_structs_src = unsafe_structs_src,
    .structs_dst = structs_dst,
    .unsafe_choices_src = unsafe_choices_src,
    .choices_dst = choices_dst,
    .dst_to_src_struct_matches = dst_to_src_struct_matches,
    .dst_to_src_choice_matches = dst_to_src_choice_matches,
    .unsafe_struct_strides_dst = unsafe_struct_strides_dst,
    .struct_queue = struct_queue_initial,
    .choice_queue = choice_queue_initial,
    .field_matches_header = field_matches_header,
    .field_matches = field_matches,
    .option_matches_header = option_matches_header,
    .option_matches_tags = option_matches_tags,
    .option_matches = option_matches,
    .current_level = sufficient_level,
    .required_level = required_level,
    .max_schema_work = max_schema_work,
  };
  SVFRT_CheckContext *ctx = &ctx_val;

  // Looping over potentially adversarial data, so limit work. #compatibility-work-entry.
  if (!SVFRT_check_work(ctx, unsafe_structs_src.count)) {
    out_result->error_code = ctx->error_code;
    return;
  }

  uint32_t entry_struct_index_src = UINT32_MAX;
  for (uint32_t i = 0; i < unsafe_structs_src.count; i++) {
    if (unsafe_structs_src.pointer[i].typeId == entry_struct_id) {
      entry_struct_index_src = i;
      break;
    }
  }

  uint32_t entry_struct_index_dst = UINT32_MAX;
  for (uint32_t i = 0; i < structs_dst.count; i++) {
    if (structs_dst.pointer[i].typeId == entry_struct_id) {
      entry_struct_index_dst = i;
      break;
    }
  }

  if (entry_struct_index_src == UINT32_MAX || entry_struct_index_dst == UINT32_MAX) {
    out_result->error_code = SVFRT_code_compatibility__entry_struct_id_not_found;
    return;
  }

  SVFRT_check_add_struct(ctx, entry_struct_index_src, entry_struct_index_dst);

  for (uint32_t i = 0; i < ctx->struct_queue.capacity + ctx->choice_queue.capacity; i++) {
    if (ctx->struct_queue.occupied > 0) {
      ctx->struct_queue.occupied--;
      SVFRT_IndexPair pair = ctx->struct_queue.pointer[ctx->struct_queue.occupied];
      SVFRT_check_struct(ctx, pair.index_src, pair.index_dst);

      if (ctx->error_code) {
        out_result->error_code = ctx->error_code;
        return;
      }
    } else if (ctx->choice_queue.occupied > 0) {
      ctx->choice_queue.occupied--;
      SVFRT_IndexPair pair = ctx->choice_queue.pointer[ctx->choice_queue.occupied];
      SVFRT_check_choice(ctx, pair.index_src, pair.index_dst);

      if (ctx->error_code) {
        out_result->error_code = ctx->error_code;
        return;
      }
    } else {
      break;
    }
  }

  if (ctx->struct_queue.occupied > 0 || ctx->choice_queue.occupied > 0) {
    out_result->error_code = SVFRT_code_compatibility_internal__queue_unhandled;
    return;
  }

  if (ctx->error_code) {
    out_result->error_code = ctx->error_code;
    return;
  }

  out_result->level = ctx->current_level;

  // We drop the "unsafe" naming here, because:
  // - for exact/binary compatibility, see #dst-stride-is-sane.
  // - for logical compatibility, see #logical-compatibility-stride-quirk.
  out_result->quirky_struct_strides_dst = ctx->unsafe_struct_strides_dst;

  if (ctx->current_level == SVFRT_compatibility_logical) {
    // #logical-compatibility-stride-quirk.
    //
    // If we have logical compatibility, then we will do a conversion, so it
    // does not make sense to store src-schema sizes here. So patch them up.
    for (size_t i = 0; i < structs_dst.count; i++) {
      out_result->quirky_struct_strides_dst.pointer[i] = structs_dst.pointer[i].size;
    }

    out_result->logical.unsafe_schema_src = unsafe_schema_src;
    out_result->logical.schema_dst = schema_dst;
    out_result->logical.unsafe_definition_src = unsafe_definition_src;
    out_result->logical.definition_dst = definition_dst;
    out_result->logical.entry_struct_index_src = entry_struct_index_src;
    out_result->logical.entry_struct_index_dst = entry_struct_index_dst;

    // We found these indices in the arrays, so they are safe to use.
    out_result->logical.unsafe_entry_struct_size_src = unsafe_structs_src.pointer[entry_struct_index_src].size;
    out_result->logical.entry_struct_size_dst = structs_dst.pointer[entry_struct_index_dst].size;

    out_result->logical.field_matches_header = field_matches_header;
    out_result->logical.field_matches = field_matches;

    out_result->logical.option_matches_header = option_matches_header;
    out_result->logical.option_matches_tags = option_matches_tags;
    out_result->logical.option_matches = option_matches;
  }
}
