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
	EVM evm = { 0 };

	// limit = 1024;

	evm_load_program_from_file(&evm, input_file_path);
	Trap trap = evm_execute_program(&evm , limit);
	evm_dump_stack(stdout, &evm);

	if (trap != TRAP_OK) {
		fprintf(stderr, "Trap activated: %s\n", trap_as_cstr(trap));
		return 1;
	}

	return 0;
}
