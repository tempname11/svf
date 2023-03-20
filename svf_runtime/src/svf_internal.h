#ifndef SVF_INTERNAL_H
#define SVF_INTERNAL_H

#include "svf_runtime.h"
#include "svf_meta.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SVFRT_RangeStructDefinition {
  SVF_META_StructDefinition *pointer;
  uint64_t count;
} SVFRT_RangeStructDefinition;

typedef struct SVFRT_RangeChoiceDefinition {
  SVF_META_ChoiceDefinition *pointer;
  uint64_t count;
} SVFRT_RangeChoiceDefinition;

typedef struct SVFRT_RangeFieldDefinition {
  SVF_META_FieldDefinition *pointer;
  uint64_t count;
} SVFRT_RangeFieldDefinition;

typedef struct SVFRT_RangeOptionDefinition {
  SVF_META_OptionDefinition *pointer;
  uint64_t count;
} SVFRT_RangeOptionDefinition;

void *SVFRT_from_pointer(
  SVFRT_RangeU8 bytes,
  SVFRT_Pointer pointer,
  size_t type_size
);

void *SVFRT_from_array(
  SVFRT_RangeU8 bytes,
  SVFRT_Array array,
  size_t type_size
);

#define SVFRT_POINTER_FROM_POINTER(bytes, pointer, type) ( \
  SVFRT_from_pointer((bytes), (pointer), sizeof(type)), \
)

#define SVFRT_RANGE_FROM_ARRAY(bytes, array, type) { \
  /*.pointer = */ (type *) SVFRT_from_array((bytes), (array), sizeof(type)), \
  /*.count = */ (array).count \
}

typedef struct SVFRT_ConversionResult {
  SVFRT_RangeU8 output_bytes;
  bool success;
} SVFRT_ConversionResult;

typedef struct SVFRT_ConversionInfo {
  SVF_META_Schema *s0;
  SVF_META_Schema *s1;
  SVFRT_RangeU8 r0;
  SVFRT_RangeU8 r1;
  uint32_t struct_index0;
  uint32_t struct_index1;
} SVFRT_ConversionInfo;

void SVFRT_convert_message(
  SVFRT_ConversionResult *out_result,
  SVFRT_ConversionInfo *info,
  SVFRT_RangeU8 entry_input_bytes,
  SVFRT_RangeU8 data_bytes,
  size_t max_recursion_depth,
  SVFRT_AllocatorFn *allocator_fn,
  void *allocator_ptr
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SVF_INTERNAL_H
