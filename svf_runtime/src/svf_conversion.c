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

// Toggle direct unaligned access to memory when converting. This invoked
// undefined behavior, but it should be fine on x64. Disabled to be safe, but
// left as an option.
//
// When @proper-alignment is done, this might be unnecessary.

// #define SVFRT_DIRECT_UNALIGNED_ACCESS

typedef struct SVFRT_ConversionContext {
  SVFRT_ConversionInfo *info;
  SVFRT_Bytes data_bytes;
  size_t max_recursion_depth;

  SVFRT_RangeStructDefinition structs0;
  SVFRT_RangeStructDefinition structs1;

  SVFRT_RangeChoiceDefinition choices0;
  SVFRT_RangeChoiceDefinition choices1;

  // For Phase 1.
  size_t allocation_needed;

  // For Phase 2.
  SVFRT_Bytes allocation;
  size_t allocated_already;

  bool internal_error;
  bool data_error;
} SVFRT_ConversionContext;

uint32_t SVFRT_get_type_size(
  SVFRT_ConversionContext *ctx,
  SVFRT_RangeStructDefinition structs,
  SVF_META_ConcreteType_enum t,
  SVF_META_ConcreteType_union *u
) {
  switch (t) {
    case SVF_META_ConcreteType_u8: {
      return 1;
    }
    case SVF_META_ConcreteType_u16: {
      return 2;
    }
    case SVF_META_ConcreteType_u32: {
      return 4;
    }
    case SVF_META_ConcreteType_u64: {
      return 8;
    }
    case SVF_META_ConcreteType_i8: {
      return 1;
    }
    case SVF_META_ConcreteType_i16: {
      return 2;
    }
    case SVF_META_ConcreteType_i32: {
      return 4;
    }
    case SVF_META_ConcreteType_i64: {
      return 8;
    }
    case SVF_META_ConcreteType_f32: {
      return 4;
    }
    case SVF_META_ConcreteType_f64: {
      return 8;
    }
    case SVF_META_ConcreteType_defined_struct: {
      uint32_t index = u->defined_struct.index;
      if (index >= structs.count) {
        ctx->internal_error = true;
        return 0;
      }
      return structs.pointer[index].size;
    }
    case SVF_META_ConcreteType_zero_sized:
    case SVF_META_ConcreteType_defined_choice:
    default: {
      ctx->internal_error = true;
      return 0;
    }
  }
}

void SVFRT_bump_size(
  SVFRT_ConversionContext *ctx,
  size_t size
) {
  // TODO @proper-alignment.
  ctx->allocation_needed += size;
}

void SVFRT_bump_structs(
  SVFRT_ConversionContext *ctx,
  size_t s1_index,
  size_t count
) {
  SVF_META_StructDefinition *s1 = ctx->structs1.pointer + s1_index;

  if (s1_index >= ctx->structs1.count) {
    ctx->internal_error = true;
    return;
  }

  // TODO @proper-alignment.
  ctx->allocation_needed += s1->size * count;
}

void SVFRT_bump_struct_contents(
  SVFRT_ConversionContext *ctx,
  size_t s0_index,
  size_t s1_index,
  SVFRT_Bytes input_bytes
);

void SVFRT_bump_type(
  SVFRT_ConversionContext *ctx,
  SVF_META_Type_enum t0,
  SVF_META_Type_union *u0,
  SVFRT_Bytes range_from,
  uint32_t offset_from,
  SVF_META_Type_enum t1,
  SVF_META_Type_union *u1
) {
  if (t0 == SVF_META_Type_concrete) {
    // Sanity check.
    if (t1 != SVF_META_Type_concrete) {
      ctx->internal_error = true;
      return;
    }

    switch (u1->concrete.type_enum) {
      case SVF_META_ConcreteType_defined_struct: {
        uint32_t inner_s0_index = u0->concrete.type_union.defined_struct.index;
        uint32_t inner_s1_index = u1->concrete.type_union.defined_struct.index;

        size_t inner_input_size = ctx->structs0.pointer[inner_s0_index].size;

        SVFRT_Bytes inner_input_range = {
          /*.pointer =*/ (uint8_t *) range_from.pointer + offset_from,
          /*.count =*/ inner_input_size,
        };

        if (inner_input_range.pointer + inner_input_range.count > range_from.pointer + range_from.count) {
          ctx->internal_error = true;
          return;
        }

        SVFRT_bump_struct_contents(
          ctx,
          inner_s0_index,
          inner_s1_index,
          inner_input_range
        );

        break;
      }
      case SVF_META_ConcreteType_defined_choice: {
        uint32_t inner_c0_index = u0->concrete.type_union.defined_choice.index;
        uint32_t inner_c1_index = u1->concrete.type_union.defined_choice.index;

        SVFRT_Bytes input_tag_bytes = {
          /*.pointer =*/ range_from.pointer + offset_from,
          /*.count =*/ SVFRT_TAG_SIZE,
        };

        if (input_tag_bytes.pointer + input_tag_bytes.count > range_from.pointer + range_from.count) {
          ctx->internal_error = true;
          return;
        }

        uint8_t input_tag = *input_tag_bytes.pointer;

        SVF_META_ChoiceDefinition *c0 = ctx->choices0.pointer + inner_c0_index;
        SVF_META_ChoiceDefinition *c1 = ctx->choices1.pointer + inner_c1_index;

        SVFRT_RangeOptionDefinition options0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->info->r0, c0->options, SVF_META_OptionDefinition);
        SVFRT_RangeOptionDefinition options1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->info->r1, c1->options, SVF_META_OptionDefinition);

        if (!options0.pointer || !options1.pointer) {
          ctx->internal_error = true;
          return;
        }

        if (input_tag >= options0.count) {
          ctx->data_error = true;
          return;
        }

        SVF_META_OptionDefinition *option0 = options0.pointer + input_tag;
        SVF_META_OptionDefinition *option1 = NULL;

        for (uint32_t i = 0; i < options1.count; i++) {
          SVF_META_OptionDefinition *option = options1.pointer + i;

          if (option->name_hash == option0->name_hash) {
            option1 = option;
            break;
          }
        }

        if (!option1) {
          ctx->internal_error = true;
          return;
        }

        SVFRT_bump_type(
          ctx,
          option0->type_enum,
          &option0->type_union,
          range_from,
          offset_from + SVFRT_TAG_SIZE,
          option1->type_enum,
          &option1->type_union
        );

        break;
      }
      default: {
        // Do nothing.
      }
    }
  }
  else if (t0 == SVF_META_Type_reference) {
    // Sanity check.
    if (t1 != SVF_META_Type_reference) {
      ctx->internal_error = true;
      return;
    }

    if ((uint64_t) offset_from + sizeof(SVFRT_Reference) > range_from.count) {
      ctx->data_error = true;
      return;
    }

    SVFRT_Reference in_reference = *((SVFRT_Reference *) (range_from.pointer + offset_from));

    switch (u1->reference.type_enum) {
      case SVF_META_ConcreteType_defined_struct: {
        // Sanity check.
        if (u0->reference.type_enum != SVF_META_ConcreteType_defined_struct) {
          ctx->internal_error = true;
          return;
        }

        uint32_t inner_s0_index = u0->reference.type_union.defined_struct.index;
        uint32_t inner_s1_index = u1->reference.type_union.defined_struct.index;

        uint32_t inner_input_size = ctx->structs0.pointer[inner_s0_index].size;

        void *inner_input_data = SVFRT_internal_from_reference(ctx->data_bytes, in_reference, inner_input_size);
        if (!inner_input_data) {
          ctx->data_error = true;
          return;
        }

        SVFRT_Bytes inner_input_range = {
          /*.pointer =*/ (uint8_t *) inner_input_data,
          /*.count =*/ inner_input_size,
        };

        SVFRT_bump_struct_contents(ctx, inner_s0_index, inner_s1_index, inner_input_range);
        SVFRT_bump_structs(ctx, inner_s1_index, 1);
        break;
      }
      case SVF_META_ConcreteType_u8: {
        SVFRT_bump_size(ctx, 1);
        break;
      }
      case SVF_META_ConcreteType_u16: {
        SVFRT_bump_size(ctx, 2);
        break;
      }
      case SVF_META_ConcreteType_u32: {
        SVFRT_bump_size(ctx, 4);
        break;
      }
      case SVF_META_ConcreteType_u64: {
        SVFRT_bump_size(ctx, 8);
        break;
      }
      case SVF_META_ConcreteType_i8: {
        SVFRT_bump_size(ctx, 1);
        break;
      }
      case SVF_META_ConcreteType_i16: {
        SVFRT_bump_size(ctx, 2);
        break;
      }
      case SVF_META_ConcreteType_i32: {
        SVFRT_bump_size(ctx, 4);
        break;
      }
      case SVF_META_ConcreteType_i64: {
        SVFRT_bump_size(ctx, 8);
        break;
      }
      case SVF_META_ConcreteType_f32: {
        SVFRT_bump_size(ctx, 4);
        break;
      }
      case SVF_META_ConcreteType_f64: {
        SVFRT_bump_size(ctx, 8);
        break;
      }
      case SVF_META_ConcreteType_defined_choice:
      case SVF_META_ConcreteType_zero_sized:
      default: {
        ctx->internal_error = true;
        return;
      }
    }
  } else if (t0 == SVF_META_Type_sequence) {
    // Sanity check.
    if (t1 != SVF_META_Type_sequence) {
      ctx->internal_error = true;
      return;
    }

    if ((uint64_t) offset_from + sizeof(SVFRT_Sequence) > range_from.count) {
      ctx->data_error = true;
      return;
    }

    SVFRT_Sequence in_sequence = *((SVFRT_Sequence *) (range_from.pointer + offset_from));

    switch (u0->sequence.element_type_enum) {
      case SVF_META_ConcreteType_defined_struct: {
        // Sanity check.
        if (u1->sequence.element_type_enum != SVF_META_ConcreteType_defined_struct) {
          ctx->internal_error = true;
          return;
        }

        uint32_t inner_s0_index = u0->sequence.element_type_union.defined_struct.index;
        uint32_t inner_s1_index = u1->sequence.element_type_union.defined_struct.index;
        size_t inner_size = ctx->structs0.pointer[inner_s0_index].size;

        void *inner_data = SVFRT_internal_from_sequence(ctx->data_bytes, in_sequence, inner_size);
        if (!inner_data) {
          ctx->data_error = true;
          return;
        }

        for (uint32_t i = 0; i < in_sequence.count; i++) {
          SVFRT_Bytes inner_range = {
            /*.pointer =*/ ((uint8_t *) inner_data) + inner_size * i,
            /*.count =*/ inner_size,
          };

          SVFRT_bump_struct_contents(ctx, inner_s0_index, inner_s1_index, inner_range);
        }
        SVFRT_bump_structs(ctx, inner_s1_index, in_sequence.count);

        break;
      }
      case SVF_META_ConcreteType_u8: {
        SVFRT_bump_size(ctx, 1 * in_sequence.count);
      }
      case SVF_META_ConcreteType_u16: {
        SVFRT_bump_size(ctx, 2 * in_sequence.count);
      }
      case SVF_META_ConcreteType_u32: {
        SVFRT_bump_size(ctx, 4 * in_sequence.count);
      }
      case SVF_META_ConcreteType_u64: {
        SVFRT_bump_size(ctx, 8 * in_sequence.count);
      }
      case SVF_META_ConcreteType_i8: {
        SVFRT_bump_size(ctx, 1 * in_sequence.count);
      }
      case SVF_META_ConcreteType_i16: {
        SVFRT_bump_size(ctx, 2 * in_sequence.count);
      }
      case SVF_META_ConcreteType_i32: {
        SVFRT_bump_size(ctx, 4 * in_sequence.count);
      }
      case SVF_META_ConcreteType_i64: {
        SVFRT_bump_size(ctx, 8 * in_sequence.count);
      }
      case SVF_META_ConcreteType_f32: {
        SVFRT_bump_size(ctx, 4 * in_sequence.count);
      }
      case SVF_META_ConcreteType_f64: {
        SVFRT_bump_size(ctx, 8 * in_sequence.count);
      }
      case SVF_META_ConcreteType_defined_choice:
      case SVF_META_ConcreteType_zero_sized:
      default: {
        ctx->internal_error = true;
        return;
      }
    }
  }
}

void SVFRT_bump_struct_contents(
  SVFRT_ConversionContext *ctx,
  size_t s0_index,
  size_t s1_index,
  SVFRT_Bytes input_bytes
) {
  if (s0_index >= ctx->structs0.count) {
    ctx->internal_error = true;
    return;
  }

  if (s1_index >= ctx->structs1.count) {
    ctx->internal_error = true;
    return;
  }

  SVF_META_StructDefinition *s0 = ctx->structs0.pointer + s0_index;
  SVF_META_StructDefinition *s1 = ctx->structs1.pointer + s1_index;

  SVFRT_RangeFieldDefinition fields0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->info->r0, s0->fields, SVF_META_FieldDefinition);
  SVFRT_RangeFieldDefinition fields1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->info->r1, s1->fields, SVF_META_FieldDefinition);
  if (!fields0.pointer || !fields1.pointer) {
    ctx->internal_error = true;
    return;
  }

  // Go over any sequences and references in the intersection between s0 and s1.
  // Sub-structs and choices may also contain these, so recurse over them.

  // @TODO @performance: N^2.
  for (uint32_t i = 0; i < fields0.count; i++) {
    SVF_META_FieldDefinition *field0 = fields0.pointer + i;

    for (uint32_t j = 0; j < fields1.count; j++) {
      SVF_META_FieldDefinition *field1 = fields1.pointer + j;
      if (field0->name_hash != field1->name_hash) {
        continue;
      }

      SVFRT_bump_type(
        ctx,
        field0->type_enum,
        &field0->type_union,
        input_bytes,
        field0->offset,
        field1->type_enum,
        &field1->type_union
      );
    }
  }
}

void SVFRT_copy_struct(
  SVFRT_ConversionContext *ctx,
  size_t recursion_depth,
  size_t s0_index,
  size_t s1_index,
  SVFRT_Bytes input_bytes,
  SVFRT_Bytes output_bytes
);

void SVFRT_copy_type(
  SVFRT_ConversionContext *ctx,
  size_t recursion_depth,
  SVF_META_Type_enum t0,
  SVF_META_Type_union *u0,
  SVFRT_Bytes range_from,
  uint32_t offset_from,
  SVF_META_Type_enum t1,
  SVF_META_Type_union *u1,
  SVFRT_Bytes range_to,
  uint32_t offset_to
);

void SVFRT_copy_size(
  SVFRT_ConversionContext *ctx,
  SVFRT_Bytes range_from,
  uint32_t offset_from,
  SVFRT_Bytes range_to,
  uint32_t offset_to,
  size_t size
) {
  SVFRT_Bytes input_bytes = {
    /*.pointer =*/ range_from.pointer + offset_from,
    /*.count =*/ size,
  };
  if (input_bytes.pointer + input_bytes.count > range_from.pointer + range_from.count) {
    ctx->internal_error = true;
    return;
  }

  SVFRT_Bytes output_bytes = {
    /*.pointer =*/ range_to.pointer + offset_to,
    /*.count =*/ size,
  };
  if (output_bytes.pointer + output_bytes.count > range_to.pointer + range_to.count) {
    ctx->internal_error = true;
    return;
  }

  SVFRT_MEMCPY(output_bytes.pointer, input_bytes.pointer, size);
}

void SVFRT_copy_concrete(
  SVFRT_ConversionContext *ctx,
  size_t recursion_depth,
  SVF_META_ConcreteType_enum t0,
  SVF_META_ConcreteType_union *u0,
  SVFRT_Bytes range_from,
  uint32_t offset_from,
  SVF_META_ConcreteType_enum t1,
  SVF_META_ConcreteType_union *u1,
  SVFRT_Bytes range_to,
  uint32_t offset_to
) {
  if (recursion_depth > ctx->max_recursion_depth) {
      ctx->data_error = true;
      return;
  }

  if (t0 == t1) {
    switch (t0) {
      case SVF_META_ConcreteType_defined_struct: {
        SVF_META_StructDefinition *s0 = ctx->structs0.pointer + u0->defined_struct.index;
        SVF_META_StructDefinition *s1 = ctx->structs1.pointer + u1->defined_struct.index;

        SVFRT_Bytes input_bytes = {
          /*.pointer =*/ range_from.pointer + offset_from,
          /*.count =*/ s0->size,
        };
        if (input_bytes.pointer + input_bytes.count > range_from.pointer + range_from.count) {
          ctx->internal_error = true;
          return;
        }

        SVFRT_Bytes output_bytes = {
          /*.pointer =*/ range_to.pointer + offset_to,
          /*.count =*/ s1->size,
        };
        if (output_bytes.pointer + output_bytes.count > range_to.pointer + range_to.count) {
          ctx->internal_error = true;
          return;
        }

        SVFRT_copy_struct(
          ctx,
          recursion_depth,
          u0->defined_struct.index,
          u1->defined_struct.index,
          input_bytes,
          output_bytes
        );
        break;
      }
      case SVF_META_ConcreteType_defined_choice: {
        SVF_META_ChoiceDefinition *c0 = ctx->choices0.pointer + u0->defined_struct.index;
        SVF_META_ChoiceDefinition *c1 = ctx->choices1.pointer + u1->defined_struct.index;

        // First, remap the tag.

        SVFRT_Bytes input_tag_bytes = {
          /*.pointer =*/ range_from.pointer + offset_from,
          /*.count =*/ SVFRT_TAG_SIZE,
        };
        if (input_tag_bytes.pointer + input_tag_bytes.count > range_from.pointer + range_from.count) {
          ctx->internal_error = true;
          return;
        }

        SVFRT_Bytes output_tag_bytes = {
          /*.pointer =*/ range_to.pointer + offset_to,
          /*.count =*/ SVFRT_TAG_SIZE,
        };
        if (output_tag_bytes.pointer + output_tag_bytes.count > range_to.pointer + range_to.count) {
          ctx->internal_error = true;
          return;
        }

        uint8_t input_tag = *input_tag_bytes.pointer;

        SVFRT_RangeOptionDefinition options0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->info->r0, c0->options, SVF_META_OptionDefinition);
        SVFRT_RangeOptionDefinition options1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->info->r1, c1->options, SVF_META_OptionDefinition);
        if (!options0.pointer || !options1.pointer) {
          ctx->internal_error = true;
          return;
        }

        SVF_META_OptionDefinition *option0 = NULL;
        for (uint32_t i = 0; i < options0.count; i++) {
          SVF_META_OptionDefinition *option = options0.pointer + i;

          if (option->index == input_tag) {
            option0 = option;
            break;
          }
        }

        if (!option0) {
          ctx->internal_error = true;
          return;
        }

        SVF_META_OptionDefinition *option1 = NULL;
        for (uint32_t i = 0; i < options1.count; i++) {
          SVF_META_OptionDefinition *option = options1.pointer + i;

          if (option->name_hash == option0->name_hash) {
            option1 = option;
            break;
          }
        }

        if (!option1) {
          ctx->internal_error = true;
          return;
        }

        *output_tag_bytes.pointer = option1->index;

        SVFRT_copy_type(
          ctx,
          recursion_depth,
          option0->type_enum,
          &option0->type_union,
          range_from,
          // TODO @proper-alignment.
          offset_from + SVFRT_TAG_SIZE,
          option1->type_enum,
          &option1->type_union,
          range_to,
          // TODO @proper-alignment.
          offset_to + SVFRT_TAG_SIZE
        );

        break;
      }
      case SVF_META_ConcreteType_u8: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 1);
        break;
      }
      case SVF_META_ConcreteType_u16: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 2);
        break;
      }
      case SVF_META_ConcreteType_u32: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 4);
        break;
      }
      case SVF_META_ConcreteType_u64: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 8);
        break;
      }
      case SVF_META_ConcreteType_i8: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 1);
        break;
      }
      case SVF_META_ConcreteType_i16: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 2);
        break;
      }
      case SVF_META_ConcreteType_i32: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 4);
        break;
      }
      case SVF_META_ConcreteType_i64: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 8);
        break;
      }
      case SVF_META_ConcreteType_f32: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 4);
        break;
      }
      case SVF_META_ConcreteType_f64: {
        SVFRT_copy_size(ctx, range_from, offset_from, range_to, offset_to, 8);
        break;
      }
      case SVF_META_ConcreteType_zero_sized: {
        // Not sure about this case.
        ctx->internal_error = true;
        return;
      }
    }
  } else {
    switch (t1) {
      case SVF_META_ConcreteType_u64: {
        uint64_t out_value = 0;
        switch (t0) {
          case SVF_META_ConcreteType_u32: {
            // U32 -> U64

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (uint64_t) *((uint32_t *) (range_from.pointer + offset_from));
            #else
            uint32_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (uint64_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_u16: {
            // U16 -> U64

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (uint64_t) *((uint16_t *) (range_from.pointer + offset_from));
            #else
            uint16_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (uint64_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_u8: {
            // U8 -> U64. Can't be misaligned.
            out_value = (uint64_t) *((uint8_t *) (range_from.pointer + offset_from));
            break;
          }
          default: {
            ctx->internal_error = true;
            return;
          }
        }

        #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
        *((uint64_t *) (range_to.pointer + offset_to)) = out_value;
        #else
        SVFRT_MEMCPY(range_to.pointer + offset_to, &out_value, sizeof(out_value));
        #endif

        break;
      }
      case SVF_META_ConcreteType_u32: {
        uint32_t out_value = 0;
        switch (t0) {
          case SVF_META_ConcreteType_u16: {
            // U16 -> U32

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (uint32_t) *((uint16_t *) (range_from.pointer + offset_from));
            #else
            uint16_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (uint32_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_u8: {
            // U8 -> U32. Can't be misaligned.
            out_value = (uint32_t) *((uint8_t *) (range_from.pointer + offset_from));
            break;
          }
          default: {
            ctx->internal_error = true;
            return;
          }
        }

        #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
        *((uint32_t *) (range_to.pointer + offset_to)) = out_value;
        #else
        SVFRT_MEMCPY(range_to.pointer + offset_to, &out_value, sizeof(out_value));
        #endif

        break;
      }
      case SVF_META_ConcreteType_u16: {
        uint16_t out_value = 0;
        switch (t0) {
          case SVF_META_ConcreteType_u8: {
            // U8 -> U16. Can't be misaligned.
            out_value = (uint16_t) *((uint8_t *) (range_from.pointer + offset_from));
            break;
          }
          default: {
            ctx->internal_error = true;
            return;
          }
        }

        #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
        *((uint16_t *) (range_to.pointer + offset_to)) = out_value;
        #else
        SVFRT_MEMCPY(range_to.pointer + offset_to, &out_value, sizeof(out_value));
        #endif

        break;
      }
      case SVF_META_ConcreteType_i64: {
        int64_t out_value = 0;
        switch (t0) {
          case SVF_META_ConcreteType_i32: {
            // I32 -> I64

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (int64_t) *((int32_t *) (range_from.pointer + offset_from));
            #else
            int32_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (int64_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_i16: {
            // I16 -> I64

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (int64_t) *((int16_t *) (range_from.pointer + offset_from));
            #else
            int16_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (int64_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_i8: {
            // I8 -> I64. Can't be misaligned.
            out_value = (int64_t) *((int8_t *) (range_from.pointer + offset_from));
            break;
          }
          case SVF_META_ConcreteType_u32: {
            // U32 -> I64

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (int64_t) *((uint32_t *) (range_from.pointer + offset_from));
            #else
            uint32_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (int64_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_u16: {
            // U16 -> I64

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (int64_t) *((uint16_t *) (range_from.pointer + offset_from));
            #else
            uint16_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (int64_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_u8: {
            // U8 -> I64. Can't be misaligned.
            out_value = (int64_t) *((uint8_t *) (range_from.pointer + offset_from));
            break;
          }
          default: {
            ctx->internal_error = true;
            return;
          }
        }

        #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
        *((int64_t *) (range_to.pointer + offset_to)) = out_value;
        #else
        SVFRT_MEMCPY(range_to.pointer + offset_to, &out_value, sizeof(out_value));
        #endif

        break;
      }
      case SVF_META_ConcreteType_i32: {
        int32_t out_value = 0;
        switch (t0) {
          case SVF_META_ConcreteType_i16: {
            // I16 -> I32

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (int32_t) *((int16_t *) (range_from.pointer + offset_from));
            #else
            int16_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (int32_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_i8: {
            // I8 -> I32. Can't be misaligned.
            out_value = (int32_t) *((int8_t *) (range_from.pointer + offset_from));
            break;
          }
          case SVF_META_ConcreteType_u16: {
            // U16 -> I32

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (int32_t) *((uint16_t *) (range_from.pointer + offset_from));
            #else
            uint16_t in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (int32_t) in_value;
            #endif

            break;
          }
          case SVF_META_ConcreteType_u8: {
            // U8 -> I32. Can't be misaligned.
            out_value = (int32_t) *((uint8_t *) (range_from.pointer + offset_from));
            break;
          }
          default: {
            ctx->internal_error = true;
            return;
          }
        }

        #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
        *((int32_t *) (range_to.pointer + offset_to)) = out_value;
        #else
        SVFRT_MEMCPY(range_to.pointer + offset_to, &out_value, sizeof(out_value));
        #endif

        break;
      }
      case SVF_META_ConcreteType_i16: {
        int16_t out_value = 0;
        switch (t0) {
          case SVF_META_ConcreteType_i8: {
            // I8 -> I16. Can't be misaligned.
            out_value = (int16_t) *((int8_t *) (range_from.pointer + offset_from));
            break;
          }
          case SVF_META_ConcreteType_u8: {
            // U8 -> I16. Can't be misaligned.
            out_value = (int16_t) *((uint8_t *) (range_from.pointer + offset_from));
            break;
          }
          default: {
            ctx->internal_error = true;
            return;
          }
        }

        #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
        *((int16_t *) (range_to.pointer + offset_to)) = out_value;
        #else
        SVFRT_MEMCPY(range_to.pointer + offset_to, &out_value, sizeof(out_value));
        #endif

        break;
      }
      case SVF_META_ConcreteType_f64: {
        // Note: not sure, if lossless integer -> float conversion should be
        // allowed or not. Probably not, because the semantics are very different.
        double out_value = 0;
        switch (t0) {
          case SVF_META_ConcreteType_f32: {
            // F32 -> F64

            #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
            out_value = (double) *((float *) (range_from.pointer + offset_from));
            #else
            float in_value;
            SVFRT_MEMCPY(&in_value, range_from.pointer + offset_from, sizeof(in_value));
            out_value = (double) in_value;
            #endif

            break;
          }
          default: {
            ctx->internal_error = true;
            return;
          }
        }

        #ifdef SVFRT_DIRECT_UNALIGNED_ACCESS
        *((double *) (range_to.pointer + offset_to)) = out_value;
        #else
        SVFRT_MEMCPY(range_to.pointer + offset_to, &out_value, sizeof(out_value));
        #endif

        break;
      }
      default: {
        ctx->internal_error = true;
        return;
      }
    }
  }
}

void SVFRT_conversion_suballocate(
  SVFRT_Bytes *out_result,
  SVFRT_ConversionContext *ctx,
  uint32_t size
) {
  // TODO @proper-alignment.

  if (ctx->allocated_already + (uint64_t) size > ctx->allocation.count) {
    return;
  }

  out_result->pointer = ctx->allocation.pointer + ctx->allocated_already;
  out_result->count = (uint64_t) size;

  ctx->allocated_already += size;
}

void SVFRT_copy_type(
  SVFRT_ConversionContext *ctx,
  size_t recursion_depth,
  SVF_META_Type_enum t0,
  SVF_META_Type_union *u0,
  SVFRT_Bytes range_from,
  uint32_t offset_from,
  SVF_META_Type_enum t1,
  SVF_META_Type_union *u1,
  SVFRT_Bytes range_to,
  uint32_t offset_to
) {
  if (t0 == SVF_META_Type_concrete) {
    if (t1 != SVF_META_Type_concrete) {
      ctx->internal_error = true;
      return;
    }

    SVFRT_copy_concrete(
      ctx,
      recursion_depth,
      u0->concrete.type_enum,
      &u0->concrete.type_union,
      range_from,
      offset_from,
      u1->concrete.type_enum,
      &u1->concrete.type_union,
      range_to,
      offset_to
    );
  } else if (t0 == SVF_META_Type_reference) {
    if (t1 != SVF_META_Type_reference) {
      ctx->internal_error = true;
      return;
    }

    if ((uint64_t) offset_from + sizeof(SVFRT_Reference) > range_from.count) {
      ctx->data_error = true;
      return;
    }

    SVFRT_Reference in_reference = *((SVFRT_Reference *) (range_from.pointer + offset_from));

    uint32_t inner_output_size = SVFRT_get_type_size(
      ctx,
      ctx->structs1,
      u1->reference.type_enum,
      &u1->reference.type_union
    );

    SVFRT_Bytes suballocation = {0};
    SVFRT_conversion_suballocate(
      &suballocation,
      ctx,
      inner_output_size
    );

    if (range_to.count < offset_to + sizeof(SVFRT_Reference)) {
      ctx->internal_error = true;
      return;
    }

    SVFRT_Reference *out_reference = (SVFRT_Reference *) (range_to.pointer + offset_to);
    out_reference->data_offset_complement = ~(suballocation.pointer - ctx->allocation.pointer);

    SVFRT_copy_concrete(
      ctx,
      recursion_depth + 1,
      u0->reference.type_enum,
      &u0->reference.type_union,
      ctx->data_bytes,
      ~in_reference.data_offset_complement,
      u1->reference.type_enum,
      &u1->reference.type_union,
      suballocation,
      0
    );
  } else if (t0 == SVF_META_Type_sequence) {
    if (t1 != SVF_META_Type_sequence) {
      ctx->internal_error = true;
      return;
    }

    if ((uint64_t) offset_from + sizeof(SVFRT_Sequence) > range_from.count) {
      ctx->data_error = true;
      return;
    }

    SVFRT_Sequence in_sequence = *((SVFRT_Sequence *) (range_from.pointer + offset_from));

    uint32_t inner_input_size = SVFRT_get_type_size(
      ctx,
      ctx->structs0,
      u0->sequence.element_type_enum,
      &u0->sequence.element_type_union
    );

    uint32_t inner_output_size = SVFRT_get_type_size(
      ctx,
      ctx->structs1,
      u1->sequence.element_type_enum,
      &u1->sequence.element_type_union
    );

    SVFRT_Bytes suballocation = {0};
    SVFRT_conversion_suballocate(
      &suballocation,
      ctx,
      // TODO @proper-alignment.
      inner_output_size * in_sequence.count
    );

    if (range_to.count < offset_to + sizeof(SVFRT_Sequence)) {
      ctx->internal_error = true;
      return;
    }

    SVFRT_Sequence *out_sequence = (SVFRT_Sequence *) (range_to.pointer + offset_to);
    out_sequence->data_offset_complement = ~(suballocation.pointer - ctx->allocation.pointer);
    out_sequence->count = in_sequence.count;

    for (uint32_t i = 0; i < in_sequence.count; i++) {
      SVFRT_copy_concrete(
        ctx,
        recursion_depth + 1,
        u0->sequence.element_type_enum,
        &u0->sequence.element_type_union,
        ctx->data_bytes,
        ~in_sequence.data_offset_complement + i * inner_input_size,
        u1->sequence.element_type_enum,
        &u1->sequence.element_type_union,
        suballocation,
        i * inner_output_size
      );
    }
  } else {
    ctx->internal_error = true;
    return;
  }
}

void SVFRT_copy_struct(
  SVFRT_ConversionContext *ctx,
  size_t recursion_depth,
  size_t s0_index,
  size_t s1_index,
  SVFRT_Bytes input_bytes,
  SVFRT_Bytes output_bytes
) {
  if (s0_index >= ctx->structs0.count) {
    ctx->internal_error = true;
    return;
  }

  if (s1_index >= ctx->structs1.count) {
    ctx->internal_error = true;
    return;
  }

  SVF_META_StructDefinition *s0 = ctx->structs0.pointer + s0_index;
  SVF_META_StructDefinition *s1 = ctx->structs1.pointer + s1_index;

  SVFRT_RangeFieldDefinition fields0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->info->r0, s0->fields, SVF_META_FieldDefinition);
  SVFRT_RangeFieldDefinition fields1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(ctx->info->r1, s1->fields, SVF_META_FieldDefinition);
  if (!fields0.pointer || !fields0.pointer) {
    ctx->internal_error = true;
    return;
  }

  // Go over the intersection between s0 and s1.

  // TODO @performance: N^2.
  for (uint32_t i = 0; i < fields0.count; i++) {
    SVF_META_FieldDefinition *field0 = fields0.pointer + i;

    for (uint32_t j = 0; j < fields1.count; j++) {
      SVF_META_FieldDefinition *field1 = fields1.pointer + j;
      if (field0->name_hash != field1->name_hash) {
        continue;
      }

      SVFRT_copy_type(
        ctx,
        recursion_depth,
        field0->type_enum,
        &field0->type_union,
        input_bytes,
        field0->offset,
        field1->type_enum,
        &field1->type_union,
        output_bytes,
        field1->offset
      );

      break;
    }
  }
}

void SVFRT_convert_message(
  SVFRT_ConversionResult *out_result,
  SVFRT_ConversionInfo *info,
  SVFRT_Bytes entry_input_bytes,
  SVFRT_Bytes data_bytes,
  size_t max_recursion_depth,
  SVFRT_AllocatorFn *allocator_fn,
  void *allocator_ptr
) {
  SVFRT_RangeStructDefinition structs0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(info->r0, info->s0->structs, SVF_META_StructDefinition);
  SVFRT_RangeStructDefinition structs1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(info->r1, info->s1->structs, SVF_META_StructDefinition);
  SVFRT_RangeChoiceDefinition choices0 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(info->r0, info->s0->choices, SVF_META_ChoiceDefinition);
  SVFRT_RangeChoiceDefinition choices1 = SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(info->r1, info->s1->choices, SVF_META_ChoiceDefinition);

  if (0
    || !structs0.pointer
    || !structs1.pointer
    || !choices0.pointer
    || !choices1.pointer
  ) {
    return;
  }

  SVFRT_ConversionContext ctx_val = {0};
  ctx_val.data_bytes = data_bytes;
  ctx_val.info = info;
  ctx_val.max_recursion_depth = max_recursion_depth;
  ctx_val.structs0 = structs0;
  ctx_val.structs1 = structs1;
  ctx_val.choices0 = choices0;
  ctx_val.choices1 = choices1;
  SVFRT_ConversionContext *ctx = &ctx_val;

  //
  // Phase 1: calculate size needed for the allocation.
  //

  SVFRT_bump_struct_contents(
    ctx,
    info->struct_index0,
    info->struct_index1,
    entry_input_bytes
  );
  SVFRT_bump_structs(
    ctx,
    info->struct_index1,
    1 // Single entry struct.
  );

  if (ctx->internal_error || ctx->data_error) {
    return;
  }

  void *allocated_pointer = allocator_fn(allocator_ptr, ctx->allocation_needed);
  if (!allocated_pointer) {
    return;
  }
  ctx->allocation.pointer = allocated_pointer;
  ctx->allocation.count = ctx->allocation_needed;

  // Zero out the memory.
  SVFRT_MEMSET(ctx->allocation.pointer, 0, ctx->allocation.count);

  //
  // Phase 2: actually copy the data.
  //

  SVF_META_StructDefinition *s1 = ctx->structs1.pointer + ctx->info->struct_index1;

  // Entry is special, as it always resides at the end of the data range.
  //
  // TODO: @proper-alignment.
  SVFRT_Bytes entry_output_bytes = {
    /*.pointer =*/ ctx->allocation.pointer + ctx->allocation.count - s1->size,
    /*.size =*/ s1->size,
  };

  SVFRT_copy_struct(
    ctx,
    0,
    info->struct_index0,
    info->struct_index1,
    entry_input_bytes,
    entry_output_bytes
  );

  // The above call will have "allocated" memory for everything preceding the
  // entry, but also copied fields of the entry struct without marking that
  // struct's memory as allocated. This is the only thing that remains.
  //
  // TODO: @proper-alignment.
  ctx->allocated_already += s1->size;

  if (ctx->allocated_already != ctx->allocation.count) {
    ctx->internal_error = true;
    return;
  }

  if (ctx->internal_error || ctx->data_error) {
    return;
  }

  out_result->success = true;
  out_result->output_bytes = ctx->allocation;
}
