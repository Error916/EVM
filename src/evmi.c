#define EVM_IMPLEMENTATION
#include "./evm.h"

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: ./evmi <input.evm>\n");
		fprintf(stderr, "ERROR: expected input and output\n");
		exit(1);
	}

	const char *input_file_path = argv[1];

	int limit = -1; // NO LIMIT
	// NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
	static EVM evm = { 0 };

	evm_load_program_from_file(&evm, input_file_path);
	evm_load_standard_natives(&evm);

	Err err = evm_execute_program(&evm , limit);

	if (err != ERR_OK) {
		fprintf(stderr, "Trap activated: %s\n", err_as_cstr(err));
		return 1;
	}

	return 0;
}
