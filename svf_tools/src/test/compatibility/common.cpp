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

  PreparedSchema result = {
    .schema_content_hash = 0,
  };

  svf::runtime::WriteContext<svf::META::SchemaDefinition> ctx = {};
  ctx.writer_fn = write_arena;
  ctx.writer_ptr = arena;
  auto schema_pointer = vm::realign(arena);

  U32 field_name_hash_delta = (params->change_field_name_hashes ? 0x10000 : 0);
  U32 field_offset_delta = (params->change_field_offsets ? 8 : 0);
  svf::META::FieldDefinition base_fields[5] = {
    {
      .nameHash = 0xFD00 + field_name_hash_delta, // "FD" for "Field Definition".
      .offset = field_offset_delta,
      .type_tag = params->change_type_tag ? svf::META::Type_tag::sequence : svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = params->change_concrete_type_tag ? svf::META::ConcreteType_tag::f64 : svf::META::ConcreteType_tag::u64,
        },
      },
    },
    {
      .nameHash = 0xFD01 + field_name_hash_delta, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(field_offset_delta + sizeof(U64)),
      .type_tag = svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::META::ConcreteType_tag::definedStruct,
          .type_payload = {
            .definedChoice = {
              .index = params->corrupt_struct_index ? U32(0xDEAD) : 1,
            }
          },
        },
      },
    },
    {
      .nameHash = 0xFD02 + field_name_hash_delta, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(field_offset_delta + sizeof(U64) + sizeof(U8)),
      .type_tag = svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::META::ConcreteType_tag::definedStruct,
          .type_payload = {
            .definedChoice = {
              .index = params->corrupt_struct_index ? U32(0xDEAD) : (params->different_struct_refs ? 2 : 1),
            }
          },
        },
      },
    },
    {
      .nameHash = 0xFD03 + field_name_hash_delta, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(field_offset_delta + sizeof(U64) + 2 * sizeof(U8)),
      .type_tag = svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::META::ConcreteType_tag::definedChoice,
          .type_payload = {
            .definedChoice = {
              .index = params->corrupt_choice_index ? U32(0xDEAD) : 0,
            }
          },
        },
      },
    },
    {
      .nameHash = 0xFD04 + field_name_hash_delta, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(field_offset_delta + sizeof(U64) + 3 * sizeof(U8)),
      .type_tag = svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::META::ConcreteType_tag::definedChoice,
          .type_payload = {
            .definedChoice = {
              .index = params->corrupt_choice_index ? U32(0xDEAD) : (params->different_choice_refs ? 1 : 0),
            }
          },
        },
      },
    },
  };
  auto fields = svf::runtime::write_sequence(&ctx, base_fields, params->less_fields ? 1 : 5);

  for (UInt i = 0; i < params->extra_fields; i++) {
    svf::META::FieldDefinition extra_field = {
      .nameHash = 0xFD0E + i + field_name_hash_delta, // "FD" for "Field Definition".
      .offset = safe_int_cast<U32>(field_offset_delta + sizeof(U64) + (4 + i) * sizeof(U8)),
      .type_tag = svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::META::ConcreteType_tag::u8,
        },
      },
    };
    svf::runtime::write_sequence_element(&ctx, &extra_field, &fields);
  }

  if (params->corrupt_fields) {
    fields.data_offset_complement = 0xDEAD;
    fields.count = 0xBEEF;
  }

  U32 end_padding = 1;
  ASSERT(!params->no_struct_end_padding || !params->extra_struct_end_padding);
  if (params->no_struct_end_padding) {
    end_padding = 0;
  }
  if (params->extra_struct_end_padding) {
    end_padding = 2;
  }

  svf::META::StructDefinition base_structs[3] = {
    {
      .nameHash = 0x5D00, // "SD" for "Struct Definition".
      .size = safe_int_cast<U32>(
        field_offset_delta
          + sizeof(U64)
          + (4 + params->extra_fields + end_padding) * sizeof(U8)
      ),
      .fields = fields,
    },
    {
      .nameHash = 0x5D01, // "SD" for "Struct Definition".
      .size = 1,
      // TODO: valid fields should be here.
    },
    {
      .nameHash = 0x5D02, // "SD" for "Struct Definition".
      .size = 1,
      // TODO: valid fields should be here.
    },
  };
  auto structs = svf::runtime::write_sequence(&ctx, base_structs, 3);

  for (UInt i = 0; i < params->extra_structs; i++) {
    svf::META::StructDefinition extra_struct = {
      .nameHash = 0x5D00 + i, // "SD" for "Struct Definition".,
      .size = 1,
      // TODO: valid fields should be here.
    };
    svf::runtime::write_sequence_element(&ctx, &extra_struct, &structs);
  }

  if (params->corrupt_structs) {
    structs.data_offset_complement = 0xDEAD;
    structs.count = 0xBEEF;
  }

  U32 option_name_hash_delta = (params->change_option_name_hashes ? 0x10000 : 0);
  U8 option_tag_delta = (params->change_option_tags ? 128 : 0);
  svf::META::OptionDefinition base_options[2] = {
    {
      .nameHash = 0x0D00 + option_name_hash_delta, // "OD" for "Option Definition".

      // Do not apply `option_tag_delta` here. We want all-zero data to be
      // valid, which means that a zero-tag option should exists.
      .tag = 0,

      .type_tag = svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::META::ConcreteType_tag::zeroSized
        },
      },
    },
    {
      .nameHash = 0x0D01 + option_name_hash_delta, // "OD" for "Option Definition".
      .tag = safe_int_cast<U8>(1 + option_tag_delta),
      .type_tag = svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::META::ConcreteType_tag::zeroSized
        },
      },
    },
  };
  auto options = svf::runtime::write_sequence(&ctx, base_options, 2);

  for (UInt i = 0; i < params->extra_options; i++) {
    svf::META::OptionDefinition extra_option = {
      .nameHash = 0x0D0E + i + option_name_hash_delta, // "OD" for "Option Definition".
      .tag = safe_int_cast<U8>(2 + i + option_tag_delta),
      .type_tag = svf::META::Type_tag::concrete,
      .type_payload = {
        .concrete = {
          .type_tag = svf::META::ConcreteType_tag::zeroSized,
        },
      },
    };
    svf::runtime::write_sequence_element(&ctx, &extra_option, &options);
  }

  if (params->corrupt_options) {
    options.data_offset_complement = 0xDEAD;
    options.count = 0xBEEF;
  }

  svf::META::ChoiceDefinition choice_definitions[2] = {
    {
      .nameHash = 0xCD00, // "CD" for "Choice Definition".
      .payloadSize = 0,
      .options = options,
    },
    {
      .nameHash = 0xCD01, // "CD" for "Choice Definition".
      .payloadSize = 0,
      .options = options,
    },
  };
  auto choices = svf::runtime::write_sequence(&ctx, choice_definitions, 2);

  if (params->corrupt_choices) {
    choices.data_offset_complement = 0xDEAD;
    choices.count = 0xBEEF;
  }

  svf::META::SchemaDefinition schema_definition = {
    .nameHash = 0xEDED, // "ED" for "Entry Definition".
    .structs = structs,
    .choices = choices,
  };
  svf::runtime::write_finish(&ctx, &schema_definition);

  ASSERT(ctx.finished);
  ASSERT(ctx.error_code == 0);

  result.schema.pointer = (U8 *) schema_pointer;
  result.schema.count = (U8 *) vm::realign(arena, 1) - (U8 *) schema_pointer;
  result.entry_stride = base_structs[0].size;
  result.entry_struct_name_hash = base_structs[0].nameHash;

  result.schema_content_hash = hash64::begin();
  hash64::add_bytes(&result.schema_content_hash, {result.schema.pointer, result.schema.count});

  return result;
}

SVFRT_Bytes prepare_message(vm::LinearArena *arena, PreparedSchema *schema) {
  auto entry_buffer = vm::many<U8>(arena, schema->entry_stride);

  auto message_pointer = vm::realign(arena);
  SVFRT_WriteContext ctx;
  SVFRT_write_start(
    &ctx,
    write_arena,
    arena,
    schema->schema_content_hash,
    schema->schema,
    schema->entry_struct_name_hash
  );
  SVFRT_write_finish(&ctx, entry_buffer.pointer, entry_buffer.count);
  ASSERT(ctx.finished);
  ASSERT(ctx.error_code == 0);

  SVFRT_Bytes message = {};
  message.pointer = (U8 *) message_pointer;
  message.count = (U8 *) vm::realign(arena, 1) - (U8 *) message_pointer;
  return message;
}
