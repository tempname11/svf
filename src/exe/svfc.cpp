// UNREVIEWED.
#include "../platform.hpp"
#include "../core.hpp"

struct CommandLineOptions {
  enum class Subcommand {
    cpp,
    binary,
  };

  Bool valid;
  Subcommand subcommand;
  Range<Byte> input_file_path; // empty if stdin.
};

CommandLineOptions parse_command_line_options(Range<Byte *> args) {
  // `svfc cpp <file>` or `svfc binary <file>`.

  CommandLineOptions result = {
    .valid = true,
  };

  if (args.count < 2) {
    printf("Error: expected subcommand.\n");
    result.valid = false;
    return result;
  }

  auto subcommand = args.pointer[1];
  if (strcmp((char const *) subcommand, "cpp") == 0) {
    result.subcommand = CommandLineOptions::Subcommand::cpp;
    if (args.count != 3) {
      printf("Error: expected input file path.\n");
      result.valid = false;
      return result;
    }
    if (strcmp((char const *) args.pointer[2], "-") != 0) {
      result.input_file_path = {
        .pointer = args.pointer[2],
        .count = strlen((char const *) args.pointer[2]),
      };
    }
  } else if (strcmp((char const *) subcommand, "binary") == 0) {
    result.subcommand = CommandLineOptions::Subcommand::binary;
    if (args.count != 3) {
      printf("Error: expected input file path.\n");
      result.valid = false;
      return result;
    }
    if (strcmp((char const *) args.pointer[2], "-") != 0) {
      result.input_file_path = {
        .pointer = args.pointer[2],
        .count = strlen((char const *) args.pointer[2]),
      };
    }
  } else {
    printf("Error: unknown subcommand '%s'.\n", subcommand);
    result.valid = false;
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

  auto arena = vm::create_linear_arena(2ull << 30);
  // never free, we will just exit the program.

  if (!arena.reserved_range.pointer) {
    printf("Error: could not create main memory arena.\n");
    return 1;
  }

  auto input_file = stdin;
  if (options.input_file_path.pointer) {
    input_file = fopen((char const *) options.input_file_path.pointer, "rb");
  }

  auto input_buffer = Range<Byte> {
    .pointer = vm::none<Byte>(&arena),
    .count = 0,
  };
  U64 input_cstr_size = 0;
  while (!feof(input_file)) {
    input_buffer.count += vm::many<Byte>(&arena, 1024).count;
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

  auto root = svf::parsing::parse_input(&arena, input);

  if (!root) {
    printf("Error: failed to parse input.\n");
    return 1;
  }

  auto ordering = svf::typechecking::check_types(root, &arena);
  if (!ordering.pointer) {
    printf("Error: failed to check types.\n");
    return 1;
  }

  auto output_arena = vm::create_linear_arena(2ull << 30);
  // never free, we will just exit the program.

  if (!output_arena.reserved_range.pointer) {
    printf("Error: could not create output memory arena.\n");
    return 1;
  }

  if (options.subcommand == CommandLineOptions::Subcommand::cpp) {
    auto output_range = svf::output_cpp::output_code(
      root,
      ordering,
      &arena,
      &output_arena
    );
    auto result = fwrite(output_range.pointer, 1, output_range.count, stdout);
    if (result != output_range.count) {
      printf("Error: failed to write output.\n");
      return 1;
    }
  } else if (options.subcommand == CommandLineOptions::Subcommand::binary) {
    auto output_range = svf::output_binary::output_bytes(
      root,
      ordering,
      &arena,
      &output_arena
    );
    auto result = fwrite(output_range.pointer, 1, output_range.count, stdout);
    if (result != output_range.count) {
      printf("Error: failed to write output.\n");
      return 1;
    }
  } else {
    ASSERT(false);
  }

  return 0;
}
