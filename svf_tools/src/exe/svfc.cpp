#include <cstdio>
#include <cstring>
#include <cinttypes>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/library.hpp>
#include <src/svf_runtime.hpp>
#include <src/svf_stdio.h>
#include "../core.hpp"

struct CommandLineOptions {
  enum class Subcommand {
    unknown = 0,
    c,
    cpp,
    binary,
  };

  Subcommand subcommand;
  Range<U8> input_file_path; // Empty, if stdin.
  Range<U8> output_file_path; // Empty, if stdin.
};

Range<U8> parse_filename(U8 *arg) {
  Range<U8> result = {};
  if (strcmp((char const *) arg, "-") != 0) {
    result = {
      .pointer = arg,
      .count = strlen((char const *) arg),
    };
  }
  return result;
}

CommandLineOptions parse_command_line_options(Range<U8 *> args) {
  CommandLineOptions result = {};

  if (args.count < 2) {
    printf("Error: expected subcommand (\"c\", \"cpp\", or \"binary\").\n");
    return result;
  }

  auto subcommand_cstr = (char const *) args.pointer[1];
  if (strcmp(subcommand_cstr, "c") == 0) {
    if (args.count != 4) {
      printf("Error: expected input/output file paths.\n");
      return result;
    }
    result.input_file_path = parse_filename(args.pointer[2]);
    result.output_file_path = parse_filename(args.pointer[3]);
    result.subcommand = CommandLineOptions::Subcommand::c;
  } else if (strcmp(subcommand_cstr, "cpp") == 0) {
    if (args.count != 4) {
      printf("Error: expected input/output file paths.\n");
      return result;
    }
    result.input_file_path = parse_filename(args.pointer[2]);
    result.output_file_path = parse_filename(args.pointer[3]);
    result.subcommand = CommandLineOptions::Subcommand::cpp;
  } else if (strcmp(subcommand_cstr, "binary") == 0) {
    if (args.count != 4) {
      printf("Error: expected input/output file paths.\n");
      return result;
    }
    result.input_file_path = parse_filename(args.pointer[2]);
    result.output_file_path = parse_filename(args.pointer[3]);
    if (!result.output_file_path.pointer) {
      printf("Writing to stdout is not allowed for `binary` subcommand.\n");
      return result;
    }
    result.subcommand = CommandLineOptions::Subcommand::binary;
  } else {
    printf("Error: unknown subcommand '%s'.\n", subcommand_cstr);
    return result;
  }

  return result;
}

struct TextLocation {
  Bool valid;
  UInt line_index; // 0-based.
  UInt column_index; // 0-based.
  Range<U8> line;
};

TextLocation get_text_location(Range<U8> text, UInt cursor) {
  UInt line_index = 0;
  UInt column_index = 0;

  UInt cursor_line_start = 0;

  if (cursor > text.count) {
    return {};
  }

  for (UInt i = 0; i < text.count; i++) {
    if (i == cursor) {
      break;
    }

    if (text.pointer[i] == '\n') {
      line_index++;
      column_index = 0;
      cursor_line_start = i + 1; // [1 .. text.count]
    } else {
      column_index++;
    }
  }

  Range<U8> line = {
    .pointer = text.pointer + cursor_line_start,
    .count = text.count - cursor_line_start,
  };

  for (UInt j = cursor; j < text.count; j++) {
    if (text.pointer[j] == '\n') {
      line.count = j - cursor_line_start;
      break;
    }
  }

  return {
    .valid = true,
    .line_index = line_index,
    .column_index = column_index,
    .line = line,
  };
}

int main(int argc, char *argv[]) {
  auto options = parse_command_line_options({
    .pointer = (U8 **) argv,
    .count = safe_int_cast<U64>(argc),
  });

  if (options.subcommand == CommandLineOptions::Subcommand::unknown) {
    // Already printed the message, just return.
    return 1;
  }

  auto arena_value = vm::create_linear_arena(1ull << 30);
  // never free, we will just exit the program.

  auto arena = &arena_value;
  if (!arena->reserved_range.pointer) {
    printf("Error: could not create main memory arena.\n");
    return 1;
  }

  auto input_file = stdin;
  if (options.input_file_path.pointer) {
    input_file = fopen((char const *) options.input_file_path.pointer, "rb");
  }

  if (!input_file) {
    printf("Error: could not open input file.\n");
    return 1;
  }

  auto input_buffer = Bytes {
    .pointer = (U8 *) vm::realign(arena),
    .count = 0,
  };
  U64 input_cstr_size = 0;

  while (!feof(input_file)) {
    input_buffer.count += vm::many<U8>(arena, 1024).count;
    input_cstr_size += fread(input_buffer.pointer, 1, 1024, input_file);

    if (ferror(input_file)) {
      printf("Error: failed to read input.\n");
      return 1;
    }
  }

  auto input = Range<U8> {
    .pointer = (U8 *) input_buffer.pointer,
    .count = input_cstr_size,
  };

  if (input_file != stdin) {
    fclose(input_file);
  }

  auto parse_result = core::parsing::parse_input(arena, input);
  if (!parse_result.root) {
    auto description = core::parsing::get_fail_code_description(parse_result.fail.code);
    printf(
      "Error: failed to parse input. %.*s\n",
      safe_int_cast<int>(description.count),
      description.pointer
    );

    auto location = get_text_location(input, parse_result.fail.cursor);
    if (location.valid) {
      printf(
        "Line: %" PRIu64 ", column: %" PRIu64 "\n\n",
        location.line_index + 1, // 1-based
        location.column_index + 1 // 1-based
      );
      for (UInt i = 0; i < location.line.count; i++) {
        printf(i == location.column_index ? "v" : "-");
      }
      printf("\n%.*s\n", safe_int_cast<int>(location.line.count), location.line.pointer);
      for (UInt i = 0; i < location.line.count; i++) {
        printf(i == location.column_index ? "^" : "-");
      }
      printf("\n");
    } else {
      printf(
        "Could not get the line and column because of an internal error, please report this. Cursor: %" PRIu64 "\n",
        parse_result.fail.cursor
      );
    }

    return 1;
  }

  auto generation_result = core::generation::as_bytes(parse_result.root, arena);
  if (!generation_result.schema.pointer) {
    // TODO: human-readable errors. This will require more failure information,
    // than just the code. First approximation for what is needed:
    // - `type_not_found`: the offending name/ID.
    // - `cyclical_dependency`: at least one of the types involved.
    // - `empty_struct`: the offending type name.
    // - `empty_choice`: the offending type name.
    // - `choice_not_allowed`: the context/reason as to why.

    printf("Error: could not generate schema. Code 0x%x\n", int(generation_result.fail_code));
    return 1;
  }

  auto schema = generation_result.schema;

  auto output_file = stdout;
  if (options.input_file_path.pointer) {
    output_file = fopen((char const *) options.output_file_path.pointer, "wb");
  }

  if (!output_file) {
    printf("Error: could not open output file.\n");
    return 1;
  }

  if (0
    || (options.subcommand == CommandLineOptions::Subcommand::c)
    || (options.subcommand == CommandLineOptions::Subcommand::cpp)
  ) {
    auto validation_result = core::validation::validate(arena, schema);

    if (!validation_result.valid) {
      printf("Internal error: could not validate the generated schema, please report this.\n");
      return 1;
    }

    Bytes output_range = {};
    if (options.subcommand == CommandLineOptions::Subcommand::cpp) {
      output_range = core::output::cpp::as_code(
        arena,
        schema,
        &validation_result
      );
    } else {
      output_range = core::output::c::as_code(
        arena,
        schema,
        &validation_result
      );
    }

    auto result = fwrite(output_range.pointer, 1, output_range.count, output_file);
    if (result != output_range.count) {
      printf("Error: failed to write output.\n");
      return 1;
    }

    fclose(output_file);
    return 0;
  } else if (options.subcommand == CommandLineOptions::Subcommand::binary) {
    // Write just the header part via `SVFRT_write_start`.
    SVFRT_WriteContext ctx;
    SVFRT_write_start(
      &ctx,
      SVFRT_fwrite,
      (void *) output_file,
      svf::Meta::_SchemaDescription::content_hash,
      {}, // Omit the schema part (here, the meta-schema).
      svf::Meta::SchemaDefinition_type_id
    );

    // Write the data part (here, the input schema).
    {
      auto result = fwrite(schema.pointer, 1, schema.count, output_file);
      if (result != schema.count) {
        printf("Error: failed to write output.\n");
        return 1;
      }
    }

    fclose(output_file);
    return 0;
  }

  return UNREACHABLE;
}
