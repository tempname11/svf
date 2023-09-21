#ifndef SVFRT_SINGLE_FILE
  #include "svf_internal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *SVFRT_from_reference(
  SVFRT_Bytes bytes,
  SVFRT_Reference reference,
  size_t type_size
) {
  uint32_t data_offset = ~reference.data_offset_complement;
  if (data_offset > bytes.count) {
    return NULL;
  }
  if (data_offset + type_size > bytes.count) {
    return NULL;
  }
  return (void *) (bytes.pointer + data_offset);
}

void *SVFRT_from_sequence(
  SVFRT_Bytes bytes,
  SVFRT_Sequence sequence,
  size_t type_size
) {
  uint32_t data_offset = ~sequence.data_offset_complement;
  if (data_offset > bytes.count) {
    return NULL;
  }
  if (data_offset + sequence.count * type_size > bytes.count) {
    return NULL;
  }
  return (void *) (bytes.pointer + data_offset);
}

#ifdef __cplusplus
} // extern "C"
#endif
