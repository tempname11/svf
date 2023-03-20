#include "svf_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

void *SVFRT_from_pointer(
  SVFRT_RangeU8 bytes,
  SVFRT_Pointer pointer,
  size_t type_size
) {
  if (pointer.data_offset > bytes.count) {
    return NULL;
  }
  if (pointer.data_offset + type_size > bytes.count) {
    return NULL;
  }
  return (void *) (bytes.pointer + pointer.data_offset);
}

void *SVFRT_from_array(
  SVFRT_RangeU8 bytes,
  SVFRT_Array array,
  size_t type_size
) {
  if (array.data_offset > bytes.count) {
    return NULL;
  }
  if (array.data_offset + array.count * type_size > bytes.count) {
    return NULL;
  }
  return (void *) (bytes.pointer + array.data_offset);
}

#ifdef __cplusplus
} // extern "C"
#endif
