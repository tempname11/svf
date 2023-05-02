#include <cstring>
#include "platform.hpp"
#include "svf.hpp"

char const input_cstr[] = R"#(#name svf_meta

Schema: struct {
  name_hash: U64;
  structs: StructDefinition[];
  choices: ChoiceDefinition[];
};

ChoiceDefinition: struct {
  name_hash: U64;
  options: OptionDefinition[];
};

OptionDefinition: struct {
  name_hash: U64;
  index: U8;
  type: Type;
};

StructDefinition: struct {
  name_hash: U64;
  fields: FieldDefinition[];
};

FieldDefinition: struct {
  name_hash: U64;
  type: Type;
};

ConcreteType: choice {
  u8;
  u16;
  u32;
  u64;
  i8;
  i16;
  i32;
  i64;
  f32;
  f64;
  zero_sized;
  defined: struct {
    top_level_definition_index: U32;
  };
};

Type: choice {
  concrete: struct {
    type: ConcreteType;
  };
  pointer: struct {
    type: ConcreteType;
  };
  flexible_array: struct {
    element_type: ConcreteType;
  };
};
)#";

struct CommandLineOptions {
  enum class Subcommand {
    cpp,
    binary,
  };

  bool valid;
  Subcommand subcommand;
};

CommandLineOptions parse_command_line_options(Range<Byte *> args) {
  // `svfc cpp` or `svfc binary`.

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
  } else if (strcmp((char const *) subcommand, "binary") == 0) {
    result.subcommand = CommandLineOptions::Subcommand::binary;
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

  static_assert(sizeof(char) == 1); // sanity check
  auto input = Range<U8> {
    .pointer = (U8 *) input_cstr,
    .count = strlen(input_cstr),
  };

  auto root = svf::parsing::parse_input(&arena, input);

  if (!root) {
    printf("Error: failed to parse input.\n");
    return 1;
  }

  if (!svf::typechecking::resolve_types(root)) {
    printf("Error: failed to resolve types.\n");
    return 1;
  }

  auto ordering = svf::typechecking::order_types(root, &arena);
  if (!ordering.pointer) {
    printf("Error: failed to order types.\n");
    return 1;
  }

  auto output_arena = vm::create_linear_arena(2ull << 30);
  // never free, we will just exit the program.

  if (!output_arena.reserved_range.pointer) {
    printf("Error: could not create output memory arena.\n");
    return 1;
  }

  if (options.subcommand == CommandLineOptions::Subcommand::cpp) {
    auto output_range = svf::output_cpp::output_code(root, ordering, &arena, &output_arena);
    auto result = fwrite(output_range.pointer, 1, output_range.count, stdout);
    if (result != output_range.count) {
      printf("Error: failed to write output.\n");
      return 1;
    }
  } else if (options.subcommand == CommandLineOptions::Subcommand::binary) {
    auto output_range = svf::output_binary::output_bytes(root, ordering, &arena, &output_arena);
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
