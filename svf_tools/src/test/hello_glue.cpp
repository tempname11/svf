#include <cstdio>
#include <cstdlib>
#include <src/library.hpp>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <generated/hpp/Hello.hpp>
#include <generated/h/Hello.h>
#include <src/svf_runtime.hpp>

extern "C" void example_write(FILE *file);
void example_read(svf::runtime::Bytes bytes);

int main(int /*argc*/, char */*argv*/[]) {
  // Write.
  {
    FILE *file = fopen("tmp_hello.svf", "wb");
    ASSERT(file);
    example_write(file);
    ASSERT(ferror(file) == 0);
    ASSERT(fclose(file) == 0);
  }

  // Read.
  {
    FILE *file = fopen("tmp_hello.svf", "rb");
    ASSERT(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    void *pointer = malloc(size);
    ASSERT((size_t) size == fread(pointer, 1, size, file));
    example_read(svf::runtime::Bytes { (U8 *) pointer, (U32) size });
    ASSERT(ferror(file) == 0);
    ASSERT(fclose(file) == 0);
  }

  return 0;
}
