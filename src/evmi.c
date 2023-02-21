#include "./evm.c"

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: ./evmi <input.evm>\n");
		fprintf(stderr, "ERROR: expected input and output\n");
		exit(1);
	}

	const char *input_file_path = argv[1];

	evm_load_program_from_file(&Global_evm, input_file_path);
	for (int i = 0; i < EVM_EXEWCUTION_LIMIT && !Global_evm.halt; ++i) {
		Trap trap = evm_execute_inst(&Global_evm);
		if (trap != TRAP_OK) {
			fprintf(stderr, "Trap activated: %s\n", trap_as_cstr(trap));
			exit(1);
		}
	}
	evm_dump_stack(stdout, &Global_evm);

	return 0;
}
