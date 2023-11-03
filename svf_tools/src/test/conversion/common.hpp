#pragma once
#include <src/library.hpp>
#include <src/svf_runtime.hpp>
#include <src/svf_meta.hpp>

U32 write_arena(void *it, SVFRT_Bytes src);
void *allocate_arena(void *it, size_t size);

struct PreparedSchema {
  SVFRT_Bytes schema;
  U64 schema_content_hash;
  U64 entry_struct_id;
  U32 entry_stride;
  SVFRT_RangeU32 struct_strides;

  // For `prepare_message`.
  U32 entry_sequence_offset;
  U32 entry_reference_offset;
  U32 entry_useq_offset;
  U32 entry_iseq_offset;
  U32 entry_fseq_offset;
  U32 entry_choice_offset;
  U32 useq_size;
  U32 iseq_size;
  U32 fseq_size;
};

struct PreparedSchemaParams {
  Bool change_leading_type;
  svf::Meta::ConcreteType_tag useq_type;
  svf::Meta::ConcreteType_tag iseq_type;
  svf::Meta::ConcreteType_tag fseq_type;
};

PreparedSchema prepare_schema(vm::LinearArena *arena, PreparedSchemaParams *params);

struct PreparedMessageParams {
  U32 sequence_count;
  U32 nested_reference_count;
  Bool invalid_sequence;
  Bool invalid_reference;
  Bool invalid_choice_tag;
  Bool alias_reference_and_sequence;
  U32 useq_count;
  Bool useq_invalid;
  U32 iseq_count;
  Bool iseq_invalid;
  U32 fseq_count;
  Bool fseq_invalid;
};

SVFRT_Bytes prepare_message(vm::LinearArena *arena, PreparedSchema *schema, PreparedMessageParams *params);
