#include <cstring>
#include "common.hpp"

Range<U8> extract_command_line_string(
	VM::LinearArena *arena,
	int argc, char *argv[]
) {
	static_assert(sizeof(char) == 1); // sanity check
	Range<U8> result = { (U8 *)arena->reserved_range.pointer, 0 };
	for (int i = 0; i < argc; i++) {
		auto argument_size_in_bytes = strlen(argv[i]);
		auto increment_count = argument_size_in_bytes + 1; // for trailing space
		auto pointer = VM::allocate_many<U8>(arena, increment_count);
		if (!pointer) {
			return {};
		}
		memcpy(pointer, argv[i], argument_size_in_bytes);
		pointer[argument_size_in_bytes] = ' '; // trailing space
		result.count += increment_count;
	}
	return result;
}

int main(int argc, char *argv[]) {
	auto arena = VM::create_linear_arena(2ull << 30);
	// never free, we will just exit the program.

	if (!arena.reserved_range.pointer) {
		printf("Error: could not create main memory arena.\n");
		return 1;
	}

	auto command_line_string = extract_command_line_string(&arena, argc, argv);

	if (!command_line_string.pointer) {
		printf("Error: could not extract command line string.\n");
	}

	// Debug.
	printf("Command line string is:\n");
	printf("%.*s\n", int(command_line_string.count), command_line_string.pointer);

	printf("Success.\n");
	return 0;
}