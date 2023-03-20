#pragma once
#include <cstring>
#include <cstdio>
#include <src/library.hpp>
#include "../core.hpp"

namespace core::output {

struct OutputContext {
  vm::LinearArena *dedicated_arena;
  meta::Schema *in_schema;
  Bytes in_bytes;
};

using Ctx = OutputContext *;

static inline
void output_cstring(Ctx ctx, const char *cstr) {
  auto str = range_from_cstr(cstr);
  auto out = vm::many<Byte>(ctx->dedicated_arena, str.count);
  range_copy(out, str);
}

static inline
void output_u8_array(Ctx ctx, svf::Array<U8> array) {
  auto out = vm::many<Byte>(ctx->dedicated_arena, array.count);
  auto data = range_subrange(ctx->in_bytes, array.data_offset, array.count);
  range_copy(out, data);
}

static inline
void output_u8_hex(Ctx ctx, U8 value) {
  char buffer[3];
  auto result = snprintf(buffer, 3, "%02X", value);
  ASSERT(result == 2);
  output_cstring(ctx, buffer);
}

static inline
void output_hex(Ctx ctx, U64 value) {
  char buffer[17]; // 16 + 1
  auto result = snprintf(buffer, 17, "%016zX", value);
  ASSERT(result == 16);
  output_cstring(ctx, buffer);
}

static inline
void output_decimal(Ctx ctx, U64 value) {
  char buffer[21]; // Python: len(str(2**64)) + 1
  auto result = snprintf(buffer, 21, "%zu", value);
  ASSERT(result > 0);
  ASSERT(result < 21);
  output_cstring(ctx, buffer);
}

static inline
void output_raw_bytes(Ctx ctx, Bytes bytes) {
  for (UInt i = 0; i < bytes.count; i+= 8) {
    UInt line_count = mini(i + 8, bytes.count) - i;
    output_cstring(ctx, "    ");
    for (UInt j = 0; j < line_count; j++) {
      output_cstring(ctx, "0x");
      output_u8_hex(ctx, bytes.pointer[i + j]);
      if (i + j != bytes.count - 1) {
        if (j == line_count - 1) {
          output_cstring(ctx, ",");
        } else {
          output_cstring(ctx, ", ");
        }
      }
    }
    output_cstring(ctx, "\n");
  }
}

} // namespace core::output
