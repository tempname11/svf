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
  SVF_META_StructDefinition *pointer;
  uint32_t count;
} SVFRT_RangeStructDefinition;

typedef struct SVFRT_RangeChoiceDefinition {
  SVF_META_ChoiceDefinition *pointer;
  uint32_t count;
} SVFRT_RangeChoiceDefinition;

typedef struct SVFRT_RangeFieldDefinition {
  SVF_META_FieldDefinition *pointer;
  uint32_t count;
} SVFRT_RangeFieldDefinition;

typedef struct SVFRT_RangeOptionDefinition {
  SVF_META_OptionDefinition *pointer;
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
