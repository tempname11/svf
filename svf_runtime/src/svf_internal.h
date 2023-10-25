#ifndef SVF_INTERNAL_H
#define SVF_INTERNAL_H

#ifndef SVFRT_SINGLE_FILE
  #include "svf_runtime.h"
  #include "svf_meta.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SVFRT_RangeStructDefinition {
  SVF_Meta_StructDefinition *pointer;
  uint32_t count;
} SVFRT_RangeStructDefinition;

typedef struct SVFRT_RangeChoiceDefinition {
  SVF_Meta_ChoiceDefinition *pointer;
  uint32_t count;
} SVFRT_RangeChoiceDefinition;

typedef struct SVFRT_RangeFieldDefinition {
  SVF_Meta_FieldDefinition *pointer;
  uint32_t count;
} SVFRT_RangeFieldDefinition;

typedef struct SVFRT_RangeOptionDefinition {
  SVF_Meta_OptionDefinition *pointer;
  uint32_t count;
} SVFRT_RangeOptionDefinition;

void *SVFRT_internal_from_reference(
  SVFRT_Bytes bytes,
  SVFRT_Reference reference,
  uint32_t type_size
);

void *SVFRT_internal_from_sequence(
  SVFRT_Bytes bytes,
  SVFRT_Sequence sequence,
  uint32_t type_size
);

#define SVFRT_INTERNAL_POINTER_FROM_REFERENCE(bytes, reference, type) ( \
  (type *) SVFRT_internal_from_reference((bytes), (reference), sizeof(type)), \
)

// Caveat: may return { NULL, count }.
// TODO: use macro tricks to force to return { NULL, 0 } in this case?
#define SVFRT_INTERNAL_RANGE_FROM_SEQUENCE(bytes, sequence, type) { \
  /*.pointer = */ (type *) SVFRT_internal_from_sequence((bytes), (sequence), sizeof(type)), \
  /*.count = */ (sequence).count \
}

typedef struct SVFRT_LogicalCompatibilityInfo {
  SVFRT_Bytes unsafe_schema_src;
  SVFRT_Bytes schema_dst;
  SVF_Meta_SchemaDefinition *unsafe_definition_src;
  SVF_Meta_SchemaDefinition *definition_dst;
  uint32_t entry_struct_index_src;
  uint32_t entry_struct_index_dst;
  uint32_t unsafe_entry_struct_size_src;
  uint32_t entry_struct_size_dst;
} SVFRT_LogicalCompatibilityInfo;

typedef struct SVFRT_CompatibilityResult {
  SVFRT_CompatibilityLevel level;
  SVFRT_RangeU32 quirky_struct_strides_dst; // See #logical-compatibility-stride-quirk.
  SVFRT_ErrorCode error_code; // See `SVFRT_code_compatibility__*`.

  SVFRT_LogicalCompatibilityInfo logical; // Only for `SVFRT_compatibility_logical`.
} SVFRT_CompatibilityResult;

// Check compatibility of two schemas, i.e. can the data written in one schema
// ("src"), be read using another schema ("dst").
//
// - `result` must be zero-initialized.
// - `scratch_memory` must have a certain size dependent on the read schema.
// See `min_read_scratch_memory_size` in the header generated from the read schema.
//
// The result may refer to scratch memory, so it needs to be kept alive as long
// as `out_result` is used.
//
// TODO: this is currently internal, but some variation of this should be public
// as part of some tentative "Compatibility Lookup API".
void SVFRT_check_compatibility(
  SVFRT_CompatibilityResult *out_result,
  SVFRT_Bytes scratch_memory,
  SVFRT_Bytes unsafe_schema_src,
  SVFRT_Bytes schema_dst,
  uint64_t entry_struct_name_hash,
  SVFRT_CompatibilityLevel required_level,
  SVFRT_CompatibilityLevel sufficient_level,
  uint32_t max_schema_work
);

typedef struct SVFRT_ConversionResult {
  SVFRT_Bytes output_bytes; // Note: may refer to allocated memory even on failure.
  bool success;
  SVFRT_ErrorCode error_code;
} SVFRT_ConversionResult;

void SVFRT_convert_message(
  SVFRT_ConversionResult *out_result,
  SVFRT_CompatibilityResult *check_result,
  SVFRT_Bytes data_bytes,
  uint32_t max_recursion_depth,
  uint32_t total_data_size_limit,
  SVFRT_AllocatorFn *allocator_fn,
  void *allocator_ptr
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SVF_INTERNAL_H
