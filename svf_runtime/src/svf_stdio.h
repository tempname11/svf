#ifndef SVF_STDIO_H
#define SVF_STDIO_H

#ifdef __cplusplus
  #include <cstdio>
#else
  #include <stdio.h>
#endif

#include "svf_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t SVFRT_fwrite(void *file, SVFRT_Bytes bytes) {
  size_t result = fwrite(bytes.pointer, 1, bytes.count, (FILE *) file);
  if (result != bytes.count) {
    // Signal error
    return 0;
  }
  if (result > (size_t) UINT32_MAX) {
    // Signal error
    return 0;
  }
  return result;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SVF_STDIO_H
