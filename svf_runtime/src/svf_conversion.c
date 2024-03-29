#ifndef SVFRT_NO_LIBC
  #ifdef __cplusplus
    #include <cstring>
  #else
    #include <string.h>
  #endif
  #define SVFRT_MEMSET memset
  #define SVFRT_MEMCPY memcpy
#else
  #if !defined(SVFRT_MEMSET) || !defined(SVFRT_MEMCPY)
    #error "When compiling with SVFRT_NO_LIBC, make sure to #define SVFRT_MEMCPY/SVFRT_MEMSET."
  #endif
#endif

#ifndef SVFRT_SINGLE_FILE
  #include "svf_runtime.h"
  #include "svf_internal.h"
#endif

// !!
//
// Important note: right now, all of the conversion code is written with the
// assumptions, that compatibility has been successfully verified between the
// schemas, and we can trust that the potentially unsafe src-schema is safe for
// our purposes. These assumptions should be referenced in the code wherever they
// are used.
//
// !!

// Note on stack usage: all recursive code in this file should be tracking the
// current recursion depth, both to prevent malicious data exploiting recursion,
// and as an attempt to prevent stack overflow generally. Since C/C++ is woefully
// lacking with regards to working with OS stack limits, this is an "attempt" and
// not a real portable solution, because on some unknown platform with small stack
// size it will still be easy to crash.
//
// The real solution would only be to reimplement the recursive calls with iteration,
// which would make maintaining a custom stack necessary. This would maximize
// the user's control, but make the API more unwieldy, probably requiring the user
// to provide their own memory for the custom stack. This would negatively affect
// ease-of-use, while providing no real benefit in most cases (and huge benefit in
// a few marginal cases, of course).
//
// TODO (low priority until a real use case happens): Reimplement with custom
// stack. There probably should be a (compile-time?) switch to use one
// implementation or the other.

// Note: after the conversion, the value tree will be suballocated parent-before-child,
// i.e. the parent's suballocation bytes come before the child suballocation bytes.
// This is contrary to how it should be, but this is never checked anywhere currently.
//
// The tree root (entry) is exempt from this, and is always placed at the end of
// the allocation, because that's how the entry is always found in the data.
//
// TODO: The suballocations should be in the right order (reversed from what is
// currently.

// Note: #unsafe-naming-semantics.
//
// Just like with compatibility checks, we should be mindful of adversarial data.
// However, it seems like in most (or all) of the code in this file we treat
// src-schema and dst-schema roughly equally, e.g. bounds checks, index checks.
// So, it's less clear why this naming is beneficial here.
//
// TODO: what would break if we treated src- and dst-schemas equally here?
// The answer to this question would make everything more clear.

typedef struct SVFRT_ConversionContext {
  SVFRT_LogicalCompatibilityInfo *info;
  SVFRT_Bytes data_bytes;
  uint32_t max_recursion_depth;

  SVFRT_RangeStructDefinition unsafe_structs_src;
  SVFRT_RangeStructDefinition structs_dst;

  SVFRT_RangeChoiceDefinition unsafe_choices_src;
  SVFRT_RangeChoiceDefinition choices_dst;

  uint32_t total_data_size_limit_dst;

  // Both for Phase 1 and 2. Should be reset in-between.
  uint32_t tally_src;
  uint32_t tally_dst;

  // For Phase 2 only.
  SVFRT_Bytes allocation;

  SVFRT_ErrorCode error_code;
} SVFRT_ConversionContext;

// Returns 0 on an error.
uint32_t SVFRT_conversion_get_type_size(
  SVFRT_RangeStructDefinition structs,
  SVF_Meta_ConcreteType_tag type_tag,
  SVF_Meta_ConcreteType_payload *type_payload
) {
  switch (type_tag) {
    case SVF_Meta_ConcreteType_tag_u8: {
      return 1;
    }
    case SVF_Meta_ConcreteType_tag_u16: {
      return 2;
    }
    case SVF_Meta_ConcreteType_tag_u32: {
      return 4;
    }
    case SVF_Meta_ConcreteType_tag_u64: {
      return 8;
    }
    case SVF_Meta_ConcreteType_tag_i8: {
      return 1;
    }
    case SVF_Meta_ConcreteType_tag_i16: {
      return 2;
    }
    case SVF_Meta_ConcreteType_tag_i32: {
      return 4;
    }
    case SVF_Meta_ConcreteType_tag_i64: {
      return 8;
    }
    case SVF_Meta_ConcreteType_tag_f32: {
      return 4;
    }
    case SVF_Meta_ConcreteType_tag_f64: {
      return 8;
    }
    case SVF_Meta_ConcreteType_tag_definedStruct: {
      uint32_t index = type_payload->definedStruct.index;
      if (index >= structs.count) {
        return 0;
      }
      return structs.pointer[index].size;
    }
    case SVF_Meta_ConcreteType_tag_nothing:
    case SVF_Meta_ConcreteType_tag_definedChoice:
    default: {
      return 0;
    }
  }
}

static inline
uint8_t SVFRT_conversion_read_uint8_t (
  SVFRT_ConversionContext *ctx,
  SVFRT_Bytes range_src,
  uint32_t unsafe_offset_src
) {
  if (unsafe_offset_src + 1 > range_src.count) {
    ctx->error_code = SVFRT_code_conversion__data_out_of_bounds;
    return 0;
  }
  // Can't be misaligned...
  return *((uint8_t *) (range_src.pointer + unsafe_offset_src));
}

static inline
uint8_t SVFRT_conversion_read_int8_t (
  SVFRT_ConversionContext *ctx,
  SVFRT_Bytes range_src,
  uint32_t unsafe_offset_src
) {
  if (unsafe_offset_src + 1 > range_src.count) {
    ctx->error_code = SVFRT_code_conversion__data_out_of_bounds;
    return 0;
  }
  // Can't be misaligned...
  return *((int8_t *) (range_src.pointer + unsafe_offset_src));
}

#define SVFRT_INSTANTIATE_READ_TYPE(type) \
  static inline \
  type SVFRT_conversion_read_##type ( \
    SVFRT_ConversionContext *ctx, \
    SVFRT_Bytes range_src, \
    uint32_t unsafe_offset_src \
  ) { \
    type value = 0; \
    if (unsafe_offset_src + sizeof(value) > range_src.count) { \
      ctx->error_code = SVFRT_code_conversion__data_out_of_bounds; \
      return value; \
    } \
    void *pointer_src = (void *) (range_src.pointer + unsafe_offset_src); \
    SVFRT_MEMCPY(&value, pointer_src, sizeof(value)); \
    return value; \
  }

SVFRT_INSTANTIATE_READ_TYPE(uint32_t)
SVFRT_INSTANTIATE_READ_TYPE(uint16_t)
SVFRT_INSTANTIATE_READ_TYPE(int32_t)
SVFRT_INSTANTIATE_READ_TYPE(int16_t)
SVFRT_INSTANTIATE_READ_TYPE(float)

#define SVFRT_INSTANTIATE_WRITE_TYPE(type) \
  static inline \
  void SVFRT_conversion_write_##type ( \
    SVFRT_ConversionContext *ctx, \
    SVFRT_Bytes range_dst, \
    uint32_t offset_dst, \
    type value \
  ) { \
    if (offset_dst + sizeof(value) > range_dst.count) { \
      ctx->error_code = SVFRT_code_conversion_internal__suballocation_out_of_bounds; \
      return; \
    } \
    void *pointer_dst = (void *) (range_dst.pointer + offset_dst); \
    SVFRT_MEMCPY(pointer_dst, &value, sizeof(value)); \
  }

SVFRT_INSTANTIATE_WRITE_TYPE(uint64_t)
SVFRT_INSTANTIATE_WRITE_TYPE(uint32_t)
SVFRT_INSTANTIATE_WRITE_TYPE(uint16_t)
SVFRT_INSTANTIATE_WRITE_TYPE(int64_t)
SVFRT_INSTANTIATE_WRITE_TYPE(int32_t)
SVFRT_INSTANTIATE_WRITE_TYPE(int16_t)
SVFRT_INSTANTIATE_WRITE_TYPE(double)

void SVFRT_conversion_copy_exact(
  SVFRT_ConversionContext *ctx,
  SVFRT_Bytes range_src,
  uint32_t unsafe_offset_src,
  SVFRT_Bytes range_dst,
  uint32_t offset_dst,
  uint32_t size
) {
  if (unsafe_offset_src + size > range_src.count) {
    ctx->error_code = SVFRT_code_conversion__data_out_of_bounds;
    return;
  }
  void *pointer_src = (void *) (range_src.pointer + unsafe_offset_src);

  if (offset_dst + size > range_dst.count) {
    ctx->error_code = SVFRT_code_conversion_internal__suballocation_out_of_bounds;
    return;
  }
  void *pointer_dst = (void *) (range_dst.pointer + offset_dst);

  SVFRT_MEMCPY(pointer_dst, pointer_src, size);
}

void SVFRT_conversion_tally(
  SVFRT_ConversionContext *ctx,
  uint32_t unsafe_size_src,
  uint32_t size_dst,
  uint32_t unsafe_count,
  SVFRT_Bytes *phase2_out_suballocation // Should be non-NULL for Phase 2.
) {
  // Prevent multiply-add overflow by casting operands to `uint64_t` first. It
  // works, because `UINT64_MAX == UINT32_MAX * UINT32_MAX + UINT32_MAX + UINT32_MAX`.
  uint64_t sum_src = (uint64_t) ctx->tally_src + (uint64_t) unsafe_size_src * (uint64_t) unsafe_count;
  uint64_t sum_dst = (uint64_t) ctx->tally_dst + (uint64_t) size_dst * (uint64_t) unsafe_count;

  if (sum_src > (uint64_t) ctx->data_bytes.count) {
    ctx->error_code = SVFRT_code_conversion__data_aliasing_detected;
    ctx->tally_src = UINT32_MAX;
    return;
  }

  if (phase2_out_suballocation) {
    // Phase2, we have the exact allocation count here.
    //
    // #phase2-reasonable-dst-sum: after this value is checked, `size_dst * unsafe_count`
    // is also proven not to overflow `uint32_t`.
    if (sum_dst > (uint64_t) ctx->allocation.count) {
      ctx->error_code = SVFRT_code_conversion_internal__suballocation_mismatch;
      ctx->tally_dst = UINT32_MAX;
      return;
    }

    // No risk of overflow, see #phase2-reasonable-dst-sum.
    phase2_out_suballocation->count = size_dst * unsafe_count;
    phase2_out_suballocation->pointer = ctx->allocation.pointer + ctx->tally_dst;
  } else {
    // Phase 1, we don't have the exact allocation count yet here.

    if (sum_dst > (uint64_t) ctx->total_data_size_limit_dst) {
      ctx->error_code = SVFRT_code_conversion__total_data_size_limit_exceeded;
      ctx->tally_dst = UINT32_MAX;
      return;
    }
  }

  // Both sums are less or equal to some `uint32_t` value, so casts are lossless.
  ctx->tally_src = (uint32_t) sum_src;
  ctx->tally_dst = (uint32_t) sum_dst;
}

typedef struct SVFRT_Phase2_TraverseAnyType {
  SVFRT_Bytes data_range_dst;
  uint32_t data_offset_dst;
} SVFRT_Phase2_TraverseAnyType;

// This declaration is needed because of recursive calls during traversal.
// `recursion_depth` should be  incremented on each call and guards against
// runaway call stacks.
void SVFRT_conversion_traverse_any_type(
  SVFRT_ConversionContext *ctx,
  uint32_t recursion_depth,
  SVFRT_Bytes data_range_src,
  uint32_t unsafe_data_offset_src,
  SVF_Meta_Type_tag unsafe_type_tag_src,
  SVF_Meta_Type_payload *unsafe_type_payload_src,
  SVF_Meta_Type_tag type_tag_dst,
  SVF_Meta_Type_payload *type_payload_dst,
  SVFRT_Phase2_TraverseAnyType *phase2
);

typedef struct SVFRT_Phase2_TraverseStruct {
  SVFRT_Bytes struct_bytes_dst;
} SVFRT_Phase2_TraverseStruct;

void SVFRT_conversion_traverse_struct(
  SVFRT_ConversionContext *ctx,
  uint32_t recursion_depth,
  uint32_t unsafe_struct_index_src,
  uint32_t struct_index_dst,
  SVFRT_Bytes struct_bytes_src,
  SVFRT_Phase2_TraverseStruct *phase2
) {
  if (unsafe_struct_index_src >= ctx->unsafe_structs_src.count) {
    ctx->error_code = SVFRT_code_conversion__bad_schema_struct_index;
    return;
  }
  SVF_Meta_StructDefinition *unsafe_definition_src = ctx->unsafe_structs_src.pointer + unsafe_struct_index_src;

  if (struct_index_dst >= ctx->structs_dst.count) {
    ctx->error_code = SVFRT_code_conversion_internal__bad_schema_struct_index;
    return;
  }
  SVF_Meta_StructDefinition *definition_dst = ctx->structs_dst.pointer + struct_index_dst;

  // Field-matches header should have valid indices for every dst-struct, so no
  // range checking is required here.
  uint32_t field_matches_index = ctx->info->field_matches_header.pointer[struct_index_dst];

  SVFRT_RangeFieldDefinition unsafe_fields_src = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    ctx->info->unsafe_schema_src,
    unsafe_definition_src->fields,
    SVF_Meta_FieldDefinition
  );
  if (!unsafe_fields_src.pointer && unsafe_fields_src.count) {
    ctx->error_code = SVFRT_code_conversion__bad_schema_field_index;
    return;
  }

  SVFRT_RangeFieldDefinition fields_dst = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    ctx->info->schema_dst,
    definition_dst->fields,
    SVF_Meta_FieldDefinition
  );
  if (!fields_dst.pointer && fields_dst.count) {
    ctx->error_code = SVFRT_code_conversion_internal__bad_schema_field_index;
    return;
  }

  // Go over all fields. We have a precomputed table of src-indices available to
  // us here.
  for (uint32_t i = 0; i < fields_dst.count; i++) {
    SVF_Meta_FieldDefinition *field_dst = fields_dst.pointer + i;

    if (field_dst->removed) {
      continue;
    }

    // Field-matches table should be valid for every possible dst-field, so no
    // range checking is required here.
    uint32_t j = ctx->info->field_matches.pointer[field_matches_index + i];

    if (j == UINT32_MAX) {
      // Field was missing, and we are allowed to zero-initialize it. Before
      // Phase 2, all data is already zero-initialized, so nothing needs to be
      // done here.
      continue;
    }

    if (j >= unsafe_fields_src.count) {
      ctx->error_code = SVFRT_code_conversion_internal__schema_field_missing;
      return;
    }

    SVF_Meta_FieldDefinition *unsafe_field_src = unsafe_fields_src.pointer + j;

    if (unsafe_field_src->fieldId != field_dst->fieldId) {
      ctx->error_code = SVFRT_code_conversion_internal__schema_field_missing;
      return;
    }

    SVFRT_Phase2_TraverseAnyType phase2_inner = {0};
    if (phase2) {
      phase2_inner.data_range_dst = phase2->struct_bytes_dst;
      phase2_inner.data_offset_dst = field_dst->offset;
    }

    SVFRT_conversion_traverse_any_type(
      ctx,
      recursion_depth,
      struct_bytes_src,
      unsafe_field_src->offset,
      unsafe_field_src->type_tag,
      &unsafe_field_src->type_payload,
      field_dst->type_tag,
      &field_dst->type_payload,
      phase2 ? &phase2_inner : NULL
    );

    if (ctx->error_code) {
      // Early exit on any error.
      return;
    }
  }
}

typedef struct SVFRT_Phase2_TraverseConcreteType {
  SVFRT_Bytes data_range_dst;
  uint32_t data_offset_dst;
} SVFRT_Phase2_TraverseConcreteType;

void SVFRT_conversion_traverse_choice(
  SVFRT_ConversionContext *ctx,
  uint32_t recursion_depth,
  uint32_t unsafe_choice_index_src,
  uint32_t choice_index_dst,
  SVFRT_Bytes data_range_src,
  uint32_t unsafe_data_offset_src,
  SVFRT_Phase2_TraverseConcreteType *phase2
) {
  // Prevent addition overflow by casting operands to `uint64_t` first.
  if ((uint64_t) unsafe_data_offset_src + (uint64_t) SVFRT_TAG_SIZE > (uint64_t) data_range_src.count) {
    ctx->error_code = SVFRT_code_conversion__data_out_of_bounds;
    return;
  }

  SVFRT_Bytes choice_tag_bytes_src = {
    /*.pointer =*/ data_range_src.pointer + unsafe_data_offset_src,
    /*.count =*/ SVFRT_TAG_SIZE
  };

  uint8_t choice_tag = *choice_tag_bytes_src.pointer;

  if (unsafe_choice_index_src >= ctx->unsafe_choices_src.count) {
    ctx->error_code = SVFRT_code_conversion__bad_schema_choice_index;
    return;
  }
  SVF_Meta_ChoiceDefinition *unsafe_definition_src = ctx->unsafe_choices_src.pointer + unsafe_choice_index_src;

  if (choice_index_dst >= ctx->choices_dst.count) {
    ctx->error_code = SVFRT_code_conversion_internal__bad_schema_choice_index;
    return;
  }
  SVF_Meta_ChoiceDefinition *definition_dst = ctx->choices_dst.pointer + choice_index_dst;

  // Option-matches header should have valid indices for every dst-choice, so
  // no range checking is required here.
  uint32_t option_matches_index = ctx->info->field_matches_header.pointer[choice_index_dst];

  SVFRT_RangeOptionDefinition unsafe_options_src = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    ctx->info->unsafe_schema_src,
    unsafe_definition_src->options,
    SVF_Meta_OptionDefinition
  );
  if (!unsafe_options_src.pointer && unsafe_options_src.count) {
    ctx->error_code = SVFRT_code_conversion__bad_schema_options;
    return;
  }

  SVFRT_RangeOptionDefinition options_dst = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    ctx->info->schema_dst,
    definition_dst->options,
    SVF_Meta_OptionDefinition
  );
  if (!options_dst.pointer && options_dst.count) {
    ctx->error_code = SVFRT_code_conversion_internal__bad_schema_options;
    return;
  }

  uint32_t src_index = UINT32_MAX;
  uint32_t dst_index = UINT32_MAX;
  for (uint32_t i = 0; i < options_dst.count; i++) {
    // Option-matches tables should be valid for every possible dst-field, so no
    // range checking is required here...
    uint8_t src_tag = ctx->info->option_matches_tags.pointer[option_matches_index + i];

    if (src_tag && src_tag == choice_tag) {
      // ...and here.
      src_index = ctx->info->option_matches.pointer[option_matches_index + i];
      dst_index = i;
    }
  }

  if (src_index == UINT32_MAX) {
    // We did not find the tag, so defaulting to the zero tag. In Phase 2, all
    // data is already zero-initialized, so nothing needs to be done here.
    return;
  }

  SVF_Meta_OptionDefinition *unsafe_option_src = unsafe_options_src.pointer + src_index;
  SVF_Meta_OptionDefinition *option_dst = options_dst.pointer + dst_index;

  // Any conversion only needs to happen, if both options are normal (not
  // removed). For Phase 2, data is already zero-initialized, so nothing needs
  // to happen here as well.
  if (!unsafe_option_src->removed && !option_dst->removed) {
    SVFRT_Phase2_TraverseAnyType phase2_inner = {0};
    if (phase2) {
      // Make sure we can at least write the tag.
      //
      // Prevent addition overflow by casting operands to `uint64_t` first.
      //
      // In this case, it looks rather silly, as we are adding _one_. But
      // proving an overflow can't happen is much harder that just checking.
      if ((uint64_t) phase2->data_offset_dst + (uint64_t) SVFRT_TAG_SIZE > (uint64_t) phase2->data_range_dst.count) {
        ctx->error_code = SVFRT_code_conversion_internal__suballocation_out_of_bounds;
        return;
      }

      // Write the tag.
      // TODO @proper-alignment: tags.
      *(phase2->data_range_dst.pointer + phase2->data_offset_dst) = option_dst->tag;

      phase2_inner.data_range_dst = phase2->data_range_dst;

      // TODO @proper-alignment: tags.
      phase2_inner.data_offset_dst = phase2->data_offset_dst + SVFRT_TAG_SIZE;
    }

    SVFRT_conversion_traverse_any_type(
      ctx,
      recursion_depth,
      data_range_src,
      // TODO: @proper-alignment: tags.
      unsafe_data_offset_src + SVFRT_TAG_SIZE,
      unsafe_option_src->type_tag,
      &unsafe_option_src->type_payload,
      option_dst->type_tag,
      &option_dst->type_payload,
      phase2 ? &phase2_inner : NULL
    );
  }
}

void SVFRT_conversion_traverse_concrete_type(
  SVFRT_ConversionContext *ctx,
  uint32_t recursion_depth,
  SVFRT_Bytes data_range_src,
  uint32_t unsafe_data_offset_src,
  SVF_Meta_ConcreteType_tag unsafe_type_tag_src,
  SVF_Meta_ConcreteType_payload *unsafe_type_payload_src,
  SVF_Meta_ConcreteType_tag type_tag_dst,
  SVF_Meta_ConcreteType_payload *type_payload_dst,
  SVFRT_Phase2_TraverseConcreteType *phase2
) {
  switch (type_tag_dst) {
    case SVF_Meta_ConcreteType_tag_definedStruct: {

      // Sanity check.
      if (unsafe_type_tag_src != SVF_Meta_ConcreteType_tag_definedStruct) {
        ctx->error_code = SVFRT_code_conversion__schema_concrete_type_tag_mismatch;
        return;
      }

      uint32_t unsafe_struct_index_src = unsafe_type_payload_src->definedStruct.index;
      uint32_t struct_index_dst = type_payload_dst->definedStruct.index;

      if (unsafe_struct_index_src >= ctx->unsafe_structs_src.count) {
        ctx->error_code = SVFRT_code_conversion__bad_schema_struct_index;
        return;
      }
      uint32_t unsafe_struct_size_src = ctx->unsafe_structs_src.pointer[unsafe_struct_index_src].size;

      // Prevent addition overflow by casting operands to `uint64_t` first.
      if ((uint64_t) unsafe_data_offset_src + (uint64_t) unsafe_struct_size_src > (uint64_t) data_range_src.count) {
        ctx->error_code = SVFRT_code_conversion__data_out_of_bounds;
        return;
      }

      SVFRT_Bytes struct_bytes_src = {
        /*.pointer =*/ (uint8_t *) data_range_src.pointer + unsafe_data_offset_src,
        /*.count =*/ unsafe_struct_size_src
      };

      SVFRT_Phase2_TraverseStruct phase2_inner = {0};
      if (phase2) {
        if (struct_index_dst >= ctx->structs_dst.count) {
          ctx->error_code = SVFRT_code_conversion_internal__bad_schema_struct_index;
          return;
        }
        uint32_t struct_size_dst = ctx->structs_dst.pointer[struct_index_dst].size;

        // Prevent addition overflow by casting operands to `uint64_t` first.
        if ((uint64_t) phase2->data_offset_dst + (uint64_t) struct_size_dst > (uint64_t) phase2->data_range_dst.count) {
          ctx->error_code = SVFRT_code_conversion_internal__suballocation_out_of_bounds;
          return;
        }

        SVFRT_Bytes struct_bytes_dst = {
          /*.pointer =*/ (uint8_t *) phase2->data_range_dst.pointer + phase2->data_offset_dst,
          /*.count =*/ struct_size_dst
        };

        phase2_inner.struct_bytes_dst = struct_bytes_dst;
      }

      SVFRT_conversion_traverse_struct(
        ctx,
        recursion_depth,
        unsafe_struct_index_src,
        struct_index_dst,
        struct_bytes_src,
        phase2 ? &phase2_inner : NULL
      );
      return;
    }
    case SVF_Meta_ConcreteType_tag_definedChoice: {
      // Sanity check.
      if (unsafe_type_tag_src != SVF_Meta_ConcreteType_tag_definedChoice) {
        ctx->error_code = SVFRT_code_conversion__schema_concrete_type_tag_mismatch;
        return;
      }

      uint32_t unsafe_choice_index_src = unsafe_type_payload_src->definedChoice.index;
      uint32_t choice_index_dst = type_payload_dst->definedChoice.index;

      SVFRT_conversion_traverse_choice(
        ctx,
        recursion_depth,
        unsafe_choice_index_src,
        choice_index_dst,
        data_range_src,
        unsafe_data_offset_src,
        phase2
      );
      return;
    }
    case SVF_Meta_ConcreteType_tag_nothing: {
      // Sanity check. This case should only arise inside choices.
      if (unsafe_type_tag_src != SVF_Meta_ConcreteType_tag_nothing) {
        ctx->error_code = SVFRT_code_conversion__schema_concrete_type_tag_mismatch;
      }
      return;
    }
    case SVF_Meta_ConcreteType_tag_u64: {
      if (!phase2) return;
      uint64_t out_value = 0;
      switch (unsafe_type_tag_src) {
        case SVF_Meta_ConcreteType_tag_u64: { // Exact case, copy and exit.
          SVFRT_conversion_copy_exact(
            ctx,
            data_range_src,
            unsafe_data_offset_src,
            phase2->data_range_dst,
            phase2->data_offset_dst,
            sizeof(uint64_t)
          );
          return;
        }
        case SVF_Meta_ConcreteType_tag_u32: { // U32 -> U64.
          out_value = (uint64_t) SVFRT_conversion_read_uint32_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u16: { // U16 -> U64.
          out_value = (uint64_t) SVFRT_conversion_read_uint16_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u8: { // U8 -> U64.
          out_value = (uint64_t) SVFRT_conversion_read_uint8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        default: {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
          return;
        }
      }

      SVFRT_conversion_write_uint64_t(ctx, phase2->data_range_dst, phase2->data_offset_dst, out_value);
      return;
    }
    case SVF_Meta_ConcreteType_tag_u32: {
      if (!phase2) return;
      uint32_t out_value = 0;
      switch (unsafe_type_tag_src) {
        case SVF_Meta_ConcreteType_tag_u32: { // Exact case, copy and exit.
          SVFRT_conversion_copy_exact(
            ctx,
            data_range_src,
            unsafe_data_offset_src,
            phase2->data_range_dst,
            phase2->data_offset_dst,
            sizeof(uint32_t)
          );
          return;
        }
        case SVF_Meta_ConcreteType_tag_u16: { // U16 -> U32.
          out_value = (uint32_t) SVFRT_conversion_read_uint16_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u8: { // U8 -> U32.
          out_value = (uint32_t) SVFRT_conversion_read_uint8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        default: {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
          return;
        }
      }

      SVFRT_conversion_write_uint32_t(ctx, phase2->data_range_dst, phase2->data_offset_dst, out_value);
      return;
    }
    case SVF_Meta_ConcreteType_tag_u16: {
      if (!phase2) return;
      uint16_t out_value = 0;
      switch (unsafe_type_tag_src) {
        case SVF_Meta_ConcreteType_tag_u16: { // Exact case, copy and exit.
          SVFRT_conversion_copy_exact(
            ctx,
            data_range_src,
            unsafe_data_offset_src,
            phase2->data_range_dst,
            phase2->data_offset_dst,
            sizeof(uint16_t)
          );
          return;
        }
        case SVF_Meta_ConcreteType_tag_u8: { // U8 -> U16.
          out_value = (uint16_t) SVFRT_conversion_read_uint8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        default: {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
          return;
        }
      }

      SVFRT_conversion_write_uint16_t(ctx, phase2->data_range_dst, phase2->data_offset_dst, out_value);
      return;
    }
    case SVF_Meta_ConcreteType_tag_u8: {
      if (!phase2) return;
      if (unsafe_type_tag_src != SVF_Meta_ConcreteType_tag_u8) {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
        return;
      }
      SVFRT_conversion_copy_exact(
        ctx,
        data_range_src,
        unsafe_data_offset_src,
        phase2->data_range_dst,
        phase2->data_offset_dst,
        sizeof(uint8_t)
      );
      return;
    }
    case SVF_Meta_ConcreteType_tag_i64: {
      if (!phase2) return;
      int64_t out_value = 0;
      switch (unsafe_type_tag_src) {
        case SVF_Meta_ConcreteType_tag_i64: { // Exact case, copy and exit.
          SVFRT_conversion_copy_exact(
            ctx,
            data_range_src,
            unsafe_data_offset_src,
            phase2->data_range_dst,
            phase2->data_offset_dst,
            sizeof(int64_t)
          );
          return;
        }
        case SVF_Meta_ConcreteType_tag_i32: { // I32 -> I64.
          out_value = (int64_t) SVFRT_conversion_read_int32_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_i16: { // I16 -> I64.
          out_value = (int64_t) SVFRT_conversion_read_int16_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_i8: { // I8 -> I64.
          out_value = (int64_t) SVFRT_conversion_read_int8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u32: { // U32 -> I64.
          out_value = (int64_t) SVFRT_conversion_read_uint32_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u16: { // U16 -> I64.
          out_value = (int64_t) SVFRT_conversion_read_uint16_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u8: { // U8 -> I64.
          out_value = (int64_t) SVFRT_conversion_read_uint8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        default: {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
          return;
        }
      }

      SVFRT_conversion_write_int64_t(ctx, phase2->data_range_dst, phase2->data_offset_dst, out_value);
      return;
    }
    case SVF_Meta_ConcreteType_tag_i32: {
      if (!phase2) return;
      int32_t out_value = 0;
      switch (unsafe_type_tag_src) {
        case SVF_Meta_ConcreteType_tag_i32: { // Exact case, copy and exit.
          SVFRT_conversion_copy_exact(
            ctx,
            data_range_src,
            unsafe_data_offset_src,
            phase2->data_range_dst,
            phase2->data_offset_dst,
            sizeof(int32_t)
          );
          return;
        }
        case SVF_Meta_ConcreteType_tag_i64: { // Exact case, copy and exit.
          SVFRT_conversion_copy_exact(
            ctx,
            data_range_src,
            unsafe_data_offset_src,
            phase2->data_range_dst,
            phase2->data_offset_dst,
            sizeof(int64_t)
          );
          return;
        }
        case SVF_Meta_ConcreteType_tag_i16: { // I16 -> I32.
          out_value = (int32_t) SVFRT_conversion_read_int16_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_i8: { // I8 -> I32.
          out_value = (int32_t) SVFRT_conversion_read_int8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u16: { // U16 -> I32.
          out_value = (int32_t) SVFRT_conversion_read_uint16_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u8: { // U8 -> I32.
          out_value = (int32_t) SVFRT_conversion_read_uint8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        default: {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
          return;
        }
      }

      SVFRT_conversion_write_int32_t(ctx, phase2->data_range_dst, phase2->data_offset_dst, out_value);
      return;
    }
    case SVF_Meta_ConcreteType_tag_i16: {
      if (!phase2) return;
      int16_t out_value = 0;
      switch (unsafe_type_tag_src) {
        case SVF_Meta_ConcreteType_tag_i16: { // Exact case, copy and exit.
          SVFRT_conversion_copy_exact(
            ctx,
            data_range_src,
            unsafe_data_offset_src,
            phase2->data_range_dst,
            phase2->data_offset_dst,
            sizeof(int16_t)
          );
          return;
        }
        case SVF_Meta_ConcreteType_tag_i8: { // I8 -> I16.
          out_value = (int16_t) SVFRT_conversion_read_int8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        case SVF_Meta_ConcreteType_tag_u8: { // U8 -> I16.
          out_value = (int16_t) SVFRT_conversion_read_uint8_t(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        default: {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
          return;
        }
      }

      SVFRT_conversion_write_int16_t(ctx, phase2->data_range_dst, phase2->data_offset_dst, out_value);
      return;
    }
    case SVF_Meta_ConcreteType_tag_i8: {
      if (!phase2) return;
      if (unsafe_type_tag_src != SVF_Meta_ConcreteType_tag_i8) {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
        return;
      }
      SVFRT_conversion_copy_exact(
        ctx,
        data_range_src,
        unsafe_data_offset_src,
        phase2->data_range_dst,
        phase2->data_offset_dst,
        sizeof(int8_t)
      );
      return;
    }
    case SVF_Meta_ConcreteType_tag_f64: {
      if (!phase2) return;
      // Note: it is unclear, whether lossless integer -> float conversions
      // should be allowed or not. Probably not, because the semantics are very
      // different.

      double out_value = 0;
      switch (unsafe_type_tag_src) {
        case SVF_Meta_ConcreteType_tag_f64: { // Exact case, copy and exit.
          SVFRT_conversion_copy_exact(
            ctx,
            data_range_src,
            unsafe_data_offset_src,
            phase2->data_range_dst,
            phase2->data_offset_dst,
            sizeof(double)
          );
          return;
        }
        case SVF_Meta_ConcreteType_tag_f32: { // F32 -> F64.
          out_value = (double) SVFRT_conversion_read_float(ctx, data_range_src, unsafe_data_offset_src);
          break;
        }
        default: {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
          return;
        }
      }

      SVFRT_conversion_write_double(ctx, phase2->data_range_dst, phase2->data_offset_dst, out_value);
      return;
    }
    case SVF_Meta_ConcreteType_tag_f32: {
      if (!phase2) return;
      if (unsafe_type_tag_src != SVF_Meta_ConcreteType_tag_f32) {
          ctx->error_code = SVFRT_code_conversion_internal__schema_incompatible_types;
        return;
      }
      SVFRT_conversion_copy_exact(
        ctx,
        data_range_src,
        unsafe_data_offset_src,
        phase2->data_range_dst,
        phase2->data_offset_dst,
        sizeof(float)
      );
      return;
    }
    default: {
      ctx->error_code = SVFRT_code_conversion__bad_type;
      return;
    }
  }
}

void SVFRT_conversion_traverse_any_type(
  SVFRT_ConversionContext *ctx,
  uint32_t recursion_depth,
  SVFRT_Bytes data_range_src,
  uint32_t unsafe_data_offset_src,
  SVF_Meta_Type_tag unsafe_type_tag_src,
  SVF_Meta_Type_payload *unsafe_type_payload_src,
  SVF_Meta_Type_tag type_tag_dst,
  SVF_Meta_Type_payload *type_payload_dst,
  SVFRT_Phase2_TraverseAnyType *phase2
) {
  recursion_depth += 1;
  if (recursion_depth > ctx->max_recursion_depth) {
    ctx->error_code = SVFRT_code_conversion__max_recursion_depth_exceeded;
    return;
  }

  switch (unsafe_type_tag_src) {
    case SVF_Meta_Type_tag_concrete: {
      // Sanity check.
      if (type_tag_dst != SVF_Meta_Type_tag_concrete) {
        ctx->error_code = SVFRT_code_conversion__schema_type_tag_mismatch;
        return;
      }

      SVFRT_Phase2_TraverseConcreteType phase2_inner = {0};
      if (phase2) {
        phase2_inner.data_range_dst = phase2->data_range_dst;
        phase2_inner.data_offset_dst = phase2->data_offset_dst;
      }

      SVFRT_conversion_traverse_concrete_type(
        ctx,
        recursion_depth,
        data_range_src,
        unsafe_data_offset_src,
        unsafe_type_payload_src->concrete.type_tag,
        &unsafe_type_payload_src->concrete.type_payload,
        type_payload_dst->concrete.type_tag,
        &type_payload_dst->concrete.type_payload,
        phase2 ? &phase2_inner : NULL
      );

      return;
    }
    case SVF_Meta_Type_tag_reference: {
      // Sanity check.
      if (type_tag_dst != SVF_Meta_Type_tag_reference) {
        ctx->error_code = SVFRT_code_conversion__schema_type_tag_mismatch;
        return;
      }

      // Prevent addition overflow by casting operands to `uint64_t` first.
      if ((uint64_t) unsafe_data_offset_src + (uint64_t) sizeof(SVFRT_Reference) > (uint64_t) data_range_src.count) {
        ctx->error_code = SVFRT_code_conversion__data_out_of_bounds;
        return;
      }

      // TODO @proper-alignment: potentially misaligned reference.
      SVFRT_Reference unsafe_representation_src = *((SVFRT_Reference *) (data_range_src.pointer + unsafe_data_offset_src));

      if (unsafe_representation_src.data_offset_complement == 0) {
        // Allow invalid references, but only if the representation is zero.

        // For Phase 1, nothing needs to be done here.
        // For Phase 2, the dst-representation is zero already, which is fine.
        return;
      }

      uint32_t unsafe_size_src = SVFRT_conversion_get_type_size(
        ctx->unsafe_structs_src,
        unsafe_type_payload_src->reference.type_tag,
        &unsafe_type_payload_src->reference.type_payload
      );
      if (unsafe_size_src == 0) {
        ctx->error_code = SVFRT_code_conversion__bad_type;
        return;
      }

      uint32_t size_dst = SVFRT_conversion_get_type_size(
        ctx->structs_dst,
        type_payload_dst->reference.type_tag,
        &type_payload_dst->reference.type_payload
      );
      if (size_dst == 0) {
        ctx->error_code = SVFRT_code_conversion_internal__bad_type;
        return;
      }

      SVFRT_Phase2_TraverseConcreteType phase2_inner = {0};
      SVFRT_conversion_tally(
        ctx,
        unsafe_size_src,
        size_dst,
        1,
        phase2 ? &phase2_inner.data_range_dst : NULL
      );
      if (ctx->error_code) {
        return;
      }

      // For Phase 2:
      // `phase2_inner.data_range_dst` was filled by `SVFRT_conversion_tally`.
      // `phase2_inner.data_offset_dst` is zero by default

      SVFRT_conversion_traverse_concrete_type(
        ctx,
        recursion_depth,
        ctx->data_bytes,
        ~unsafe_representation_src.data_offset_complement,
        unsafe_type_payload_src->reference.type_tag,
        &unsafe_type_payload_src->reference.type_payload,
        type_payload_dst->reference.type_tag,
        &type_payload_dst->reference.type_payload,
        phase2 ? &phase2_inner : NULL
      );
      return;
    }
    case SVF_Meta_Type_tag_sequence: {
      // Sanity check.
      if (type_tag_dst != SVF_Meta_Type_tag_sequence) {
        ctx->error_code = SVFRT_code_conversion__schema_type_tag_mismatch;
        return;
      }

      // Prevent addition overflow by casting operands to `uint64_t` first.
      if ((uint64_t) unsafe_data_offset_src + (uint64_t) sizeof(SVFRT_Sequence) > (uint64_t) data_range_src.count) {
        ctx->error_code = SVFRT_code_conversion__data_out_of_bounds;
        return;
      }

      // TODO @proper-alignment: potentially misaligned sequence.
      SVFRT_Sequence unsafe_representation_src = *((SVFRT_Sequence *) (data_range_src.pointer + unsafe_data_offset_src));

      // Allow invalid sequences, but only if the representation is zero.
      if (unsafe_representation_src.data_offset_complement == 0 && unsafe_representation_src.count == 0) {
        // For Phase 1, nothing needs to be done here.
        // For Phase 2, the dst-representation is zero already, which is fine.
        return;
      }

      uint32_t unsafe_size_src = SVFRT_conversion_get_type_size(
        ctx->unsafe_structs_src,
        unsafe_type_payload_src->reference.type_tag,
        &unsafe_type_payload_src->reference.type_payload
      );
      if (unsafe_size_src == 0) {
        ctx->error_code = SVFRT_code_conversion__bad_type;
        return;
      }

      uint32_t size_dst = SVFRT_conversion_get_type_size(
        ctx->structs_dst,
        type_payload_dst->reference.type_tag,
        &type_payload_dst->reference.type_payload
      );
      if (size_dst == 0) {
        ctx->error_code = SVFRT_code_conversion_internal__bad_type;
        return;
      }

      SVFRT_Phase2_TraverseConcreteType phase2_inner = {0};
      SVFRT_conversion_tally(
        ctx,
        unsafe_size_src,
        size_dst,
        unsafe_representation_src.count,
        phase2 ? &phase2_inner.data_range_dst : NULL
      );
      if (ctx->error_code) {
        return;
      }

      uint32_t data_offset = ~unsafe_representation_src.data_offset_complement;
      uint64_t unsafe_end_offset_src = (
        (uint64_t) data_offset +
        (uint64_t) unsafe_representation_src.count * (uint64_t) unsafe_size_src
      );

      // Prevent multiply-add overflow by casting operands to `uint64_t` first. It
      // works, because `UINT64_MAX == UINT32_MAX * UINT32_MAX + UINT32_MAX + UINT32_MAX`.
      if (unsafe_end_offset_src > (uint64_t) UINT32_MAX) {
        ctx->error_code = SVFRT_code_conversion__data_out_of_bounds;
        return;
      }

      for (uint32_t i = 0; i < unsafe_representation_src.count; i++) {
        // No overflow possible, see the `unsafe_end_offset_src` check above.
        uint32_t unsafe_final_offset_src = (
          ~unsafe_representation_src.data_offset_complement +
          i * unsafe_size_src
        );

        if (phase2) {
          // `phase2_inner.data_range_dst` was filled by `SVFRT_conversion_tally`.

          // No overflow possible:
          //
          // Let `C = unsafe_representation_src.count`.  We know that `i < C`.
          // And we know that `size_dst * C` does not overflow, because
          // `SVFRT_conversion_tally` has succeeded, see #phase2-reasonable-dst-sum.
          phase2_inner.data_offset_dst = size_dst * i;
        }

        // TODO: for a sequence of primitives, this is far from optimal. If the
        // types match, we can just `memcpy` the whole thing instead of looping.
        //
        // Typical example would be a long U8 sequence.

        SVFRT_conversion_traverse_concrete_type(
          ctx,
          recursion_depth,
          ctx->data_bytes,
          unsafe_final_offset_src,
          unsafe_type_payload_src->reference.type_tag,
          &unsafe_type_payload_src->reference.type_payload,
          type_payload_dst->reference.type_tag,
          &type_payload_dst->reference.type_payload,
          phase2 ? &phase2_inner : NULL
        );

        if (ctx->error_code) {
          // Exit early on any error. This would include any tally limit checks,
          // so it's fine to loop even though the count might be malicious.
          return;
        }
      }
      return;
    }
    default: {
      ctx->error_code = SVFRT_code_conversion__bad_schema_type_tag;
    }
  }
}

void SVFRT_convert_message(
  SVFRT_ConversionResult *out_result,
  SVFRT_CompatibilityResult *check_result,
  SVFRT_Bytes data_bytes,
  uint32_t max_recursion_depth,
  uint32_t total_data_size_limit,
  SVFRT_AllocatorFn *allocator_fn, // Non-NULL.
  void *allocator_ptr
) {
  // A previously passed compatibility check allows us to reuse some data, and
  // also make some assumptions.
  if (check_result->level != SVFRT_compatibility_logical) {
    out_result->error_code = SVFRT_code_conversion_internal__need_logical_compatibility;
    return;
  }

  SVFRT_LogicalCompatibilityInfo *info = &check_result->logical;

  uint32_t unsafe_entry_struct_size = info->unsafe_entry_struct_size_src;

  if (unsafe_entry_struct_size > data_bytes.count) {
    out_result->error_code = SVFRT_code_conversion__data_out_of_bounds;
    return;
  }

  // Now, `unsafe_entry_size` can be considered safe.
  // TODO @proper-alignment: struct access.
  SVFRT_Bytes entry_bytes_src = {
    /*.pointer =*/ data_bytes.pointer + data_bytes.count - unsafe_entry_struct_size,
    /*.count =*/ unsafe_entry_struct_size,
  };

  SVFRT_RangeStructDefinition unsafe_structs_src = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    info->unsafe_schema_src,
    info->unsafe_definition_src->structs,
    SVF_Meta_StructDefinition
  );
  if (!unsafe_structs_src.pointer && unsafe_structs_src.count) {
    out_result->error_code = SVFRT_code_conversion__bad_schema_structs;
    return;
  };

  SVFRT_RangeStructDefinition structs_dst = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    info->schema_dst,
    info->definition_dst->structs,
    SVF_Meta_StructDefinition
  );
  if (!structs_dst.pointer && structs_dst.count) {
    out_result->error_code = SVFRT_code_conversion_internal__bad_schema_structs;
    return;
  }

  SVFRT_RangeChoiceDefinition unsafe_choices_src = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    info->unsafe_schema_src,
    info->unsafe_definition_src->choices,
    SVF_Meta_ChoiceDefinition
  );
  if (!unsafe_choices_src.pointer && unsafe_choices_src.count) {
    out_result->error_code = SVFRT_code_conversion__bad_schema_choices;
    return;
  };

  SVFRT_RangeChoiceDefinition choices_dst = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(
    info->schema_dst,
    info->definition_dst->choices,
    SVF_Meta_ChoiceDefinition
  );
  if (!choices_dst.pointer && choices_dst.count) {
    out_result->error_code = SVFRT_code_conversion_internal__bad_schema_choices;
    return;
  }

  SVFRT_ConversionContext ctx_val = {0};
  ctx_val.info = info;
  ctx_val.data_bytes = data_bytes;
  ctx_val.max_recursion_depth = max_recursion_depth;
  ctx_val.unsafe_structs_src = unsafe_structs_src;
  ctx_val.structs_dst = structs_dst;
  ctx_val.unsafe_choices_src = unsafe_choices_src;
  ctx_val.choices_dst = choices_dst;
  ctx_val.total_data_size_limit_dst = total_data_size_limit;
  SVFRT_ConversionContext *ctx = &ctx_val;

  //
  // Phase 1: calculate size needed for the allocation.
  //

  uint32_t recursion_depth = 0;
  SVFRT_conversion_traverse_struct(
    ctx,
    recursion_depth,
    info->entry_struct_index_src,
    info->entry_struct_index_dst,
    entry_bytes_src,
    NULL
  );
  if (ctx->error_code) {
    out_result->error_code = ctx->error_code;
    return;
  }

  SVFRT_conversion_tally(
    ctx,
    info->unsafe_entry_struct_size_src,
    info->entry_struct_size_dst,
    1,
    NULL
  );
  if (ctx->error_code) {
    out_result->error_code = ctx->error_code;
    return;
  }

  void *allocated_pointer = allocator_fn(allocator_ptr, ctx->tally_dst);
  if (!allocated_pointer) {
    out_result->error_code = SVFRT_code_conversion__allocation_failed;
    return;
  }

  ctx->allocation.pointer = (uint8_t *) allocated_pointer;
  ctx->allocation.count = ctx->tally_dst;
  out_result->output_bytes = ctx->allocation;

  // Zero out the memory.
  SVFRT_MEMSET(ctx->allocation.pointer, 0, ctx->allocation.count);

  // Reset tallies between the phases.
  ctx->tally_src = 0;
  ctx->tally_dst = 0;
  recursion_depth = 0;

  //
  // Phase 2: actually copy the data.
  //

  SVF_Meta_StructDefinition *definition_dst = ctx->structs_dst.pointer + ctx->info->entry_struct_index_dst;

  // Entry is special, as it always resides at the end of the data range.
  //
  // TODO: @proper-alignment: struct access.
  SVFRT_Bytes entry_bytes_dst = {
    /*.pointer =*/ ctx->allocation.pointer + ctx->allocation.count - definition_dst->size,
    /*.size =*/ definition_dst->size,
  };

  SVFRT_Phase2_TraverseStruct phase2 = {
    /*.struct_bytes_dst =*/ entry_bytes_dst
  };
  SVFRT_conversion_traverse_struct(
    ctx,
    recursion_depth,
    info->entry_struct_index_src,
    info->entry_struct_index_dst,
    entry_bytes_src,
    &phase2
  );
  if (ctx->error_code) {
    out_result->error_code = ctx->error_code;
    return;
  }

  SVFRT_Bytes entry_bytes_dst_alternate = {0};
  SVFRT_conversion_tally(
    ctx,
    info->unsafe_entry_struct_size_src,
    info->entry_struct_size_dst,
    1,
    &entry_bytes_dst_alternate
  );
  if (ctx->error_code) {
    out_result->error_code = ctx->error_code;
    return;
  }

  // Sanity checks.
  if (
    (entry_bytes_dst.pointer != entry_bytes_dst_alternate.pointer) ||
    (entry_bytes_dst.count != entry_bytes_dst_alternate.count) ||
    (ctx->tally_dst != ctx->allocation.count)
  ) {
    // Something went terribly wrong.
    // TODO: should this kind of "fatal" error be separate?
    out_result->error_code = SVFRT_code_conversion_internal__suballocation_mismatch;
    return;
  }

  out_result->success = true;
}
