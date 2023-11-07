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
};

struct PreparedSchemaParams {
  Bool corrupt_structs;
  Bool corrupt_choices;
  Bool corrupt_fields;
  Bool corrupt_options;
  Bool corrupt_struct_index;
  Bool corrupt_choice_index;

  U32 extra_structs;
  U32 extra_fields;
  U32 extra_options;
  Bool less_fields;
  Bool less_options;
  Bool change_field_ids;
  Bool change_field_offsets;
  Bool change_option_ids;
  Bool change_option_tags;
  Bool change_type_tag;
  Bool change_concrete_type_tag;
  Bool no_struct_end_padding;
  Bool extra_struct_end_padding;
  Bool different_struct_refs;
  Bool different_choice_refs;
};

PreparedSchema prepare_schema(vm::LinearArena *arena, PreparedSchemaParams *params);
SVFRT_Bytes prepare_message(vm::LinearArena *arena, PreparedSchema *schema);
