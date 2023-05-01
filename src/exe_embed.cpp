#include <cstdio>
#include <memory>

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s <input file> <output file>\n", argv[0]);
		return 1;
	}

	auto input_file = fopen(argv[1], "rb");
	if (!input_file) {
		return 1;
	}

	auto output_file = fopen(argv[2], "w");
	if (!output_file) {
		return 1;
	}

	if (fseek(input_file, 0, SEEK_END) != 0) {
		return 1;
	}

	auto tell = ftell(input_file);
	if (tell == -1) {
		return 1;
	}
	auto input_file_size = (size_t) tell;

	if (fseek(input_file, 0, SEEK_SET) != 0) {
		return 1;
	}

	static_assert(sizeof(char) == 1);
	auto input_file_buffer = (char *) malloc(input_file_size);
	if (!input_file_buffer) {
		return 1;
	}

	if (fread(input_file_buffer, 1, input_file_size, input_file) != input_file_size) {
		return 1;
	}

	if (fclose(input_file) != 0) {
		return 1;
	}

	if (fprintf(output_file, "unsigned char const bytes[] = {") < 0) {
		return 1;
	}

	for (auto i = 0; i < input_file_size; i++) {
		if (i % 8 == 0) {
			if (fprintf(output_file, "\n  ") < 0) {
				return 1;
			}
		}

		if (fprintf(output_file, "0x%02X, ", (unsigned char) input_file_buffer[i]) < 0) {
			return 1;
		}
	}

	if (fprintf(output_file, "\n};\n") < 0) {
		return 1;
	}

	if (fprintf(output_file, "size_t const size = %zu;\n", input_file_size) < 0) {
		return 1;
	}

	if (fflush(output_file) != 0) {
		return 1;
	}
	
	return 0;
}