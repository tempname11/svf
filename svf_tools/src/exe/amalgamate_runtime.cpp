#include <cstdio>
#include <src/library.hpp>

struct OutputContext {
  FILE *output_file;
  bool error;
};

void output_string(OutputContext *ctx, char const *string) {
  fwrite(string, 1, strlen(string), ctx->output_file);

  if (ferror(ctx->output_file)) {
    ctx->error = true;
  }
}

void include_file(OutputContext *ctx, char const *input_file_name) {
  output_string(ctx, "\n");
  output_string(ctx, "// ================================\n");
  output_string(ctx, "// INCLUDING FILE: ");
  output_string(ctx, input_file_name);
  output_string(ctx, "\n");
  output_string(ctx, "// ================================\n");
  output_string(ctx, "\n");

  auto input_file = fopen(input_file_name, "rb");
  if (!input_file) {
    return;
  }

  while (!feof(input_file)) {
    Byte buffer[1024];
    auto read = fread(buffer, 1, 1024, input_file);
    fwrite(buffer, 1, read, ctx->output_file);
  }

  if (ferror(input_file)) {
    ctx->error = true;
    return;
  }

  if (ferror(ctx->output_file)) {
    ctx->error = true;
    return;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    return 1;
  }

  auto output_file_name = argv[1];
  auto output_file = fopen(output_file_name, "wb");
  if (!output_file) {
    return 1;
  }

  OutputContext ctx_val = {
    .output_file = output_file,
  };
  auto ctx = &ctx_val;

  output_string(ctx, "// svf.h - alpha version - amalgamated from sources.\n");
  output_string(ctx, "//\n");
  output_string(ctx, "// https://github.com/tempname11/svf\n");
  output_string(ctx, "//\n");
  output_string(ctx, "// SVF is a format for structured data that is focused on being.\n");
  output_string(ctx, "// both machine-friendly, and human-friendly — in that order.\n");
  output_string(ctx, "//\n");
  output_string(ctx, "// Please see the LICENSE at the end of the file.\n");
  output_string(ctx, "\n");
  output_string(ctx, "#ifndef SVFRT_SINGLE_FILE_H\n");
  output_string(ctx, "#define SVFRT_SINGLE_FILE_H\n");
  output_string(ctx, "\n");
  output_string(ctx, "#define SVFRT_SINGLE_FILE\n");

  include_file(ctx, "svf_runtime.h");
  include_file(ctx, "svf_runtime.hpp");
  include_file(ctx, "svf_meta.h");
  include_file(ctx, "svf_internal.h");
  include_file(ctx, "svf_stdio.h");

  output_string(ctx, "#ifdef SVF_IMPLEMENTATION\n");
  output_string(ctx, "\n");

  include_file(ctx, "svf_compatibility.c");
  include_file(ctx, "svf_conversion.c");
  include_file(ctx, "svf_internal.c");
  include_file(ctx, "svf_runtime.c");

  output_string(ctx, "\n");
  output_string(ctx, "#endif // SVF_IMPLEMENTATION\n");
  output_string(ctx, "\n");
  output_string(ctx, "#endif // SVFRT_SINGLE_FILE_H\n");

  output_string(ctx, "\n");
  output_string(ctx, "/*");
  include_file(ctx, "../../LICENSE.txt");
  output_string(ctx, "*/\n");

  if (ctx->error) {
    return 1;
  }

  if (fclose(output_file) != 0) {
    return 1;
  }

  return 0;
}
