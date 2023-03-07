#define EVM_IMPLEMENTATION
#include "./evm.h"

EASM easm = { 0 };

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: ./easm <input.easm> <output.evm>\n");
		fprintf(stderr, "ERROR: expected input and output\n");
		exit(1);
	}

	const char *input_file_path = argv[1];
	const char *output_file_path = argv[2];

	easm_translate_source(&easm, cstr_as_sv(input_file_path), 0);
	easm_save_to_file(&easm, output_file_path);

	return 0;
}

