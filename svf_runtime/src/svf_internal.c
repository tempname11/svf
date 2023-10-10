#ifndef SVFRT_SINGLE_FILE
  #include "svf_internal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *SVFRT_internal_from_reference(
  SVFRT_Bytes bytes,
  SVFRT_Reference reference,
  uint32_t type_size
) {
  uint32_t data_offset = ~reference.data_offset_complement;

  // Prevent addition overflow by casting operands to `uint64_t` first.
  if ((uint64_t) data_offset + (uint64_t) type_size > (uint64_t) bytes.count) {
    return NULL;
  }

  return (void *) (bytes.pointer + data_offset);
}

void *SVFRT_internal_from_sequence(
  SVFRT_Bytes bytes,
  SVFRT_Sequence sequence,
  uint32_t type_stride
) {
  uint32_t data_offset = ~sequence.data_offset_complement;

  // Prevent multiply-add overflow by casting operands to `uint64_t` first. It
  // works, because `UINT64_MAX == UINT32_MAX * UINT32_MAX + UINT32_MAX + UINT32_MAX`.
  uint64_t end_offset = (uint64_t) data_offset + (
    (uint64_t) sequence.count * (uint64_t) type_stride
  );

  // Check end of the range.
  if (end_offset > (uint64_t) bytes.count) {
    return NULL;
  }
  return (void *) (bytes.pointer + data_offset);
}

#ifdef __cplusplus
} // extern "C"
#endif
