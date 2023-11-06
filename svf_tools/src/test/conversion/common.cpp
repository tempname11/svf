#include "common.hpp"

U32 write_arena(void *it, SVFRT_Bytes src) {
  auto arena = (vm::LinearArena *) it;
  auto dst = vm::many<U8>(arena, src.count);
  range_copy(dst, {src.pointer, src.count});
  return safe_int_cast<U32>(src.count);
};

void *allocate_arena(void *it, size_t size) {
  auto arena = (vm::LinearArena *) it;
  return (void *) vm::many<U8>(arena, size).pointer;
};

PreparedSchema prepare_schema(vm::LinearArena *arena, PreparedSchemaParams *params) {
  PreparedSchemaParams empty = {};
  if (!params) {
    params = &empty;
  }

  // Make sure params that are zero, are sane.
  static_assert(int(svf::Meta::ConcreteType_tag::u8) == 0);
  if (params->iseq_type == svf::Meta::ConcreteType_tag::u8) {
    params->iseq_type = svf::Meta::ConcreteType_tag::i8;
  }
  if (params->fseq_type == svf::Meta::ConcreteType_tag::u8) {
    params->fseq_type = svf::Meta::ConcreteType_tag::f32;
  }

  auto struct_strides = vm::many<U32>(arena, 2);

  svf::runtime::WriteContext<svf::Meta::SchemaDefinition> ctx = {};
  ctx.writer_fn = write_arena;
  ctx.writer_ptr = arena;
  auto schema_pointer = vm::realign(arena);
  auto leading_type_tag = (
    params->change_leading_type
      ? svf::Meta::ConcreteType_tag::u32
      : svf::Meta::ConcreteType_tag::u64
  );
  U32 leading_type_size = (
    params->change_leading_type
      ? sizeof(U32)
      : sizeof(U64)
  );

  svf::Meta::FieldDefinition base_fields[7] = {
    {
      .fieldId = 0xFD00, // "FD" for "Field Definition".
      .offset = 0,
      .type_tag = svf::Meta::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = leading_type_tag,
        },
      },
    },
    {
      .fieldId = 0xFD01, // "FD" for "Field Definition".
      .offset = leading_type_size,
      .type_tag = svf::Meta::Type_tag::reference,
      .type_payload = {
        .concrete = {
          .type_tag = svf::Meta::ConcreteType_tag::definedStruct,
          .type_payload = {
            .definedStruct = {
              .index = 1,
            }
          },
        },
      },
    },
    {
      .fieldId = 0xFD02, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(leading_type_size + sizeof(SVFRT_Reference)),
      .type_tag = svf::Meta::Type_tag::sequence,
      .type_payload = {
        .concrete = {
          .type_tag = svf::Meta::ConcreteType_tag::definedStruct,
          .type_payload = {
            .definedStruct = {
              .index = 1,
            }
          },
        },
      },
    },
    {
      .fieldId = 0xFD03, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(leading_type_size + sizeof(SVFRT_Reference) + sizeof(SVFRT_Sequence)),
      .type_tag = svf::Meta::Type_tag::sequence,
      .type_payload = {
        .concrete = {
          .type_tag = params->useq_type,
        },
      },
    },
    {
      .fieldId = 0xFD04, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(leading_type_size + sizeof(SVFRT_Reference) + 2 * sizeof(SVFRT_Sequence)),
      .type_tag = svf::Meta::Type_tag::sequence,
      .type_payload = {
        .concrete = {
          .type_tag = params->iseq_type,
        },
      },
    },
    {
      .fieldId = 0xFD05, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(leading_type_size + sizeof(SVFRT_Reference) + 3 * sizeof(SVFRT_Sequence)),
      .type_tag = svf::Meta::Type_tag::sequence,
      .type_payload = {
        .concrete = {
          .type_tag = params->fseq_type,
        },
      },
    },
    {
      .fieldId = 0xFD06, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(leading_type_size + sizeof(SVFRT_Reference) + 4 * sizeof(SVFRT_Sequence)),
      .type_tag = svf::Meta::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::Meta::ConcreteType_tag::definedChoice,
          .type_payload = {
            .definedChoice = {
              .index = 0,
            }
          },
        },
      },
    },
  };
  auto entry_fields_sequence = svf::runtime::write_sequence(&ctx, base_fields, 7);

  svf::Meta::FieldDefinition selfref_fields[1] = {
    {
      .fieldId = 0xFD10, // "FD" for "Field Definition".
      .offset = 0,
      .type_tag = svf::Meta::Type_tag::reference,
      .type_payload = {
        .concrete = {
          .type_tag = svf::Meta::ConcreteType_tag::definedStruct,
          .type_payload = {
            .definedStruct = {
              .index = 1,
            }
          },
        },
      },
    },
  };
  auto selfref_fields_sequence = svf::runtime::write_sequence(&ctx, selfref_fields, 1);

  svf::Meta::StructDefinition base_structs[2] = {
    {
      .typeId = 0x5D00, // "SD" for "Struct Definition".
      // TODO @proper-alignment: test assumes packed data, need to fix this.
      // There are other places in this file as well.
      .size = safe_int_cast<U32>(leading_type_size + sizeof(SVFRT_Reference) + 4 * sizeof(SVFRT_Sequence) + sizeof(U8)),
      .fields = entry_fields_sequence,
    },
    {
      .typeId = 0x5D01, // "SD" for "Struct Definition".
      .size = sizeof(SVFRT_Reference),
      .fields = selfref_fields_sequence,
    },
  };
  auto structs = svf::runtime::write_sequence(&ctx, base_structs, 2);

  ASSERT(struct_strides.count == 2);
  for (UInt i = 0; i < 2; i++) {
    struct_strides.pointer[i] = base_structs[i].size;
  }

  svf::Meta::OptionDefinition base_options[2] = {
    {
      .optionId = 0x0D00, // "OD" for "Option Definition".
      .tag = 0,
      .type_tag = svf::Meta::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::Meta::ConcreteType_tag::zeroSized
        },
      },
    },
    {
      .optionId = 0x0D01, // "OD" for "Option Definition".
      .tag = 1,
      .type_tag = svf::Meta::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::Meta::ConcreteType_tag::zeroSized
        },
      },
    },
  };
  auto options = svf::runtime::write_sequence(&ctx, base_options, 2);

  svf::Meta::ChoiceDefinition choice_definitions[1] = {
    {
      .typeId = 0xCD00, // "CD" for "Choice Definition".
      .payloadSize = 0,
      .options = options,
    },
  };
  auto choices = svf::runtime::write_sequence(&ctx, choice_definitions, 1);

  svf::Meta::SchemaDefinition schema_definition = {
    .schemaId = 0xEDED, // "ED" for "Entry Definition".
    .structs = structs,
    .choices = choices,
  };
  svf::runtime::write_finish(&ctx, &schema_definition);

  ASSERT(ctx.finished);
  ASSERT(ctx.error_code == 0);

  PreparedSchema result = {};
  result.schema.pointer = (U8 *) schema_pointer;
  result.schema.count = (U8 *) vm::realign(arena, 1) - (U8 *) schema_pointer;
  result.entry_stride = base_structs[0].size;
  result.entry_struct_id = base_structs[0].typeId;
  result.entry_reference_offset = base_fields[1].offset;
  result.entry_sequence_offset = base_fields[2].offset;
  result.entry_useq_offset = base_fields[3].offset;
  result.entry_iseq_offset = base_fields[4].offset;
  result.entry_fseq_offset = base_fields[5].offset;
  result.entry_choice_offset = base_fields[6].offset;
  result.struct_strides = { struct_strides.pointer, safe_int_cast<U32>(struct_strides.count) };

  if (params->useq_type == svf::Meta::ConcreteType_tag::u8) {
    result.useq_size = 1;
  } else if (params->useq_type == svf::Meta::ConcreteType_tag::u16) {
    result.useq_size = 2;
  } else if (params->useq_type == svf::Meta::ConcreteType_tag::u32) {
    result.useq_size = 4;
  } else if (params->useq_type == svf::Meta::ConcreteType_tag::u64) {
    result.useq_size = 8;
  } else {
    ASSERT(false);
  }

  if (params->iseq_type == svf::Meta::ConcreteType_tag::i8) {
    result.iseq_size = 1;
  } else if (params->iseq_type == svf::Meta::ConcreteType_tag::i16) {
    result.iseq_size = 2;
  } else if (params->iseq_type == svf::Meta::ConcreteType_tag::i32) {
    result.iseq_size = 4;
  } else if (params->iseq_type == svf::Meta::ConcreteType_tag::i64) {
    result.iseq_size = 8;
  } else {
    ASSERT(false);
  }

  if (params->fseq_type == svf::Meta::ConcreteType_tag::f32) {
    result.fseq_size = 4;
  } else if (params->fseq_type == svf::Meta::ConcreteType_tag::f64) {
    result.fseq_size = 8;
  } else {
    ASSERT(false);
  }

  result.schema_content_hash = hash64::begin();
  hash64::add_bytes(&result.schema_content_hash, {result.schema.pointer, result.schema.count});

  return result;
}

SVFRT_Bytes prepare_message(vm::LinearArena *arena, PreparedSchema *schema, PreparedMessageParams *params) {
  PreparedMessageParams empty = {};
  if (!params) {
    params = &empty;
  }

  auto entry_buffer = vm::many<U8>(arena, schema->entry_stride);

  auto message_pointer = vm::realign(arena);
  SVFRT_WriteContext ctx;
  SVFRT_write_start(
    &ctx,
    write_arena,
    arena,
    schema->schema_content_hash,
    schema->schema,
    schema->entry_struct_id
  );

  SVFRT_Sequence sequence = {};
  for (U32 i = 0; i < params->sequence_count; i++) {
    SVFRT_Reference item = {};
    SVFRT_write_sequence_element(&ctx, &item, sizeof(item), &sequence);
  }

  if (params->invalid_sequence) {
    sequence.data_offset_complement = 0xDEAD;
  }

  SVFRT_Reference reference = {};

  for (U32 i = 0; i < params->nested_reference_count; i++) {
    reference = SVFRT_write_reference(&ctx, &reference, sizeof(reference));
  }

  if (params->invalid_reference) {
    reference.data_offset_complement = 0xDEAD;
  }

  if (params->alias_reference_and_sequence) {
    ASSERT(!params->invalid_reference);
    reference.data_offset_complement = sequence.data_offset_complement;
  }

  SVFRT_Sequence useq = {};
  for (U32 i = 0; i < params->useq_count; i++) {
    U64 item = 0;
    ASSERT(schema->useq_size <= sizeof(item));
    SVFRT_write_sequence_element(&ctx, &item, schema->useq_size, &useq);
  }

  if (params->useq_invalid) {
    useq.data_offset_complement = 0xBEEF;
  }

  SVFRT_Sequence iseq = {};
  for (U32 i = 0; i < params->iseq_count; i++) {
    U64 item = 0;
    ASSERT(schema->iseq_size <= sizeof(item));
    SVFRT_write_sequence_element(&ctx, &item, schema->iseq_size, &iseq);
  }

  if (params->iseq_invalid) {
    iseq.data_offset_complement = 0xBEEF;
  }

  SVFRT_Sequence fseq = {};
  for (U32 i = 0; i < params->fseq_count; i++) {
    U64 item = 0;
    ASSERT(schema->fseq_size <= sizeof(item));
    SVFRT_write_sequence_element(&ctx, &item, schema->fseq_size, &fseq);
  }

  if (params->fseq_invalid) {
    fseq.data_offset_complement = 0xBEEF;
  }

  memcpy(entry_buffer.pointer + schema->entry_reference_offset, &reference, sizeof(reference));
  memcpy(entry_buffer.pointer + schema->entry_sequence_offset, &sequence, sizeof(sequence));
  memcpy(entry_buffer.pointer + schema->entry_useq_offset, &useq, sizeof(useq));
  memcpy(entry_buffer.pointer + schema->entry_iseq_offset, &iseq, sizeof(iseq));
  memcpy(entry_buffer.pointer + schema->entry_fseq_offset, &fseq, sizeof(fseq));

  if (params->invalid_choice_tag) {
    entry_buffer.pointer[schema->entry_choice_offset] = 0xFF;
  }

  SVFRT_write_finish(&ctx, entry_buffer.pointer, entry_buffer.count);
  ASSERT(ctx.finished);
  ASSERT(ctx.error_code == 0);

  return {
    .pointer = (U8 *) message_pointer,
    .count = safe_int_cast<U32>((U8 *) vm::realign(arena, 1) - (U8 *) message_pointer),
  };
}
