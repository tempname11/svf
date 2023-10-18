#include <cstdio>
#include <cstring>
#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/library.hpp>
#include <src/svf_runtime.hpp>
#include "../core.hpp"

namespace meta = svf::META;

struct CommandLineOptions {
  enum class Subcommand {
    c,
    cpp,
    binary,
  };

  Bool valid;
  Subcommand subcommand;
  Range<U8> input_file_path; // Empty, if stdin.
  Range<U8> output_file_path; // Empty, if stdin.
};

Range<U8> parse_filename(Byte *arg) {
  Range<U8> result = {};
  if (strcmp((char const *) arg, "-") != 0) {
    result = {
      .pointer = arg,
      .count = strlen((char const *) arg),
    };
  }
  return result;
}

CommandLineOptions parse_command_line_options(Range<Byte *> args) {
  CommandLineOptions result = {};

  if (args.count < 2) {
    printf("Error: expected subcommand (\"c\", \"cpp\", or \"binary\").\n");
    return result;
  }

  auto subcommand_cstr = (char const *) args.pointer[1];
  if (strcmp(subcommand_cstr, "c") == 0) {
    result.subcommand = CommandLineOptions::Subcommand::c;
    if (args.count != 4) {
      printf("Error: expected input/output file paths.\n");
      return result;
    }
    result.input_file_path = parse_filename(args.pointer[2]);
    result.output_file_path = parse_filename(args.pointer[3]);
    result.valid = true;
  } else if (strcmp(subcommand_cstr, "cpp") == 0) {
    result.subcommand = CommandLineOptions::Subcommand::cpp;
    if (args.count != 4) {
      printf("Error: expected input/output file paths.\n");
      return result;
    }
    result.input_file_path = parse_filename(args.pointer[2]);
    result.output_file_path = parse_filename(args.pointer[3]);
    result.valid = true;
  } else if (strcmp(subcommand_cstr, "binary") == 0) {
    result.subcommand = CommandLineOptions::Subcommand::binary;
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
    result.valid = true;
  } else {
    printf("Error: unknown subcommand '%s'.\n", subcommand_cstr);
    return result;
  }

  return result;
}

int main(int argc, char *argv[]) {
  auto options = parse_command_line_options({
    .pointer = (Byte **) argv,
    .count = safe_int_cast<U64>(argc),
  });

  if (!options.valid) {
    return 1;
  }

  auto arena_ = vm::create_linear_arena(2ull << 30);
  // never free, we will just exit the program.

  auto arena = &arena_;
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
    .pointer = (Byte *) vm::realign(arena),
    .count = 0,
  };
  U64 input_cstr_size = 0;

  while (!feof(input_file)) {
    input_buffer.count += vm::many<Byte>(arena, 1024).count;
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

  auto root = core::parsing::parse_input(arena, input);
  if (!root) {
    printf("Error: failed to parse input.\n");
    return 1;
  }

  auto schema_bytes = core::generation::as_bytes(root, arena);
  if (!schema_bytes.pointer) {
    printf("Error: could not generate schema.\n");
    return 1;
  }

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
    auto validation_result = core::validation::validate(arena, schema_bytes);

    if (!validation_result.valid) {
      printf("Error: could not validate generated schema.\n");
      return 1;
    }

    Bytes output_range = {};
    if (options.subcommand == CommandLineOptions::Subcommand::cpp) {
      output_range = core::output::cpp::as_code(
        arena,
        schema_bytes,
        &validation_result
      );
    } else {
      output_range = core::output::c::as_code(
        arena,
        schema_bytes,
        &validation_result
      );
    }

    auto result = fwrite(output_range.pointer, 1, output_range.count, output_file);
    if (result != output_range.count) {
      printf("Error: failed to write output.\n");
      return 1;
    }
    fclose(output_file);
  } else if (options.subcommand == CommandLineOptions::Subcommand::binary) {
    // TODO: use `write_start` instead here for the header & schema parts. The
    // data part will have to be written using `fwrite` anyway.
    //
    // TODO: use content hash only in the schema part.

    // Write the header.
    {
      svf::runtime::MessageHeader header = {
        .magic = { 'S', 'V', 'F' },
        .version = 0,
        .schema_length = (uint32_t) meta::binary::size,
        .entry_struct_name_hash = meta::SchemaDefinition_name_hash,
      };

      auto result = fwrite(&header, 1, sizeof(header), output_file);
      if (result != sizeof(header)) {
        printf("Error: failed to write output.\n");
        return 1;
      }
    }

    // Write the schema part (metaschema).
    {
      auto result = fwrite(meta::binary::array, 1, meta::binary::size, output_file);
      if (result != meta::binary::size) {
        printf("Error: failed to write output.\n");
        return 1;
      }
    }

    // Align.
    {
      uint8_t zeros[SVFRT_MESSAGE_PART_ALIGNMENT] = {0};
      size_t misaligned = meta::binary::size % SVFRT_MESSAGE_PART_ALIGNMENT;
      auto result = fwrite(zeros, 1, SVFRT_MESSAGE_PART_ALIGNMENT - misaligned, output_file);
      if (result != SVFRT_MESSAGE_PART_ALIGNMENT - misaligned) {
        printf("Error: failed to write output.\n");
        return 1;
      }
    }

    // Write the data part (input schema).
    {
      auto result = fwrite(schema_bytes.pointer, 1, schema_bytes.count, output_file);
      if (result != schema_bytes.count) {
        printf("Error: failed to write output.\n");
        return 1;
      }
    }

    fclose(output_file);
  } else {
    ASSERT(false);
  }

  return 0;
}
