#ifndef SVF_STDIO_H
#define SVF_STDIO_H

#ifndef SVFRT_NO_LIBC

#ifdef __cplusplus
  #include <cstdio>
#else
  #include <stdio.h>
#endif

#ifndef SVFRT_SINGLE_FILE
  #include "svf_runtime.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline
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

#endif // SVFRT_NO_LIBC

#endif // SVF_STDIO_H
