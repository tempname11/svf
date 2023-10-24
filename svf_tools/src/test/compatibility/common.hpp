#pragma once
#include <src/library.hpp>
#include <src/svf_runtime.hpp>
#include <src/svf_meta.hpp>

U32 write_arena(void *it, SVFRT_Bytes src);
void *allocate_arena(void *it, size_t size);

struct PreparedSchema {
  SVFRT_Bytes schema;
  U64 schema_content_hash;
  U64 entry_struct_name_hash;
  U32 entry_stride;
};

struct PreparedSchemaParams {
  bool corrupt_structs;
  bool corrupt_choices;
  bool corrupt_fields;
  bool corrupt_options;
  bool corrupt_struct_index;
  bool corrupt_choice_index;

  U32 extra_structs;
  U32 extra_fields;
  U32 extra_options;
  bool less_fields;
  bool change_field_name_hashes;
  bool change_field_offsets;
  bool change_option_name_hashes;
  bool change_option_tags;
  bool change_type_tag;
  bool change_concrete_type_tag;
  bool no_struct_end_padding;
  bool extra_struct_end_padding;
  bool different_struct_refs;
  bool different_choice_refs;
};

PreparedSchema prepare_schema(vm::LinearArena *arena, PreparedSchemaParams *params);
SVFRT_Bytes prepare_message(vm::LinearArena *arena, PreparedSchema *schema);
