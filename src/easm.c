#define EVM_IMPLEMENTATION
#include "./evm.h"

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: ./easm <input.easm> <output.evm>\n");
		fprintf(stderr, "ERROR: expected input and output\n");
		exit(1);
	}

	EVM evm = { 0 };
	Label_Table lt = { 0 };

	const char *input_file_path = argv[1];
	const char *output_file_path = argv[2];

	evm_translate_source(&evm, &lt, cstr_as_sv(input_file_path), 0);
	evm_save_program_to_file(&evm, output_file_path);

	return 0;
}

