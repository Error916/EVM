#include "./evm.c"

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: ./easm <input.easm> <output.evm>\n");
		fprintf(stderr, "ERROR: expected input and output\n");
		exit(1);
	}

	const char *input_file_path = argv[1];
	const char *output_file_path = argv[2];

	String_View source = slurp_file(input_file_path);
	Global_evm.program_size = evm_transalte_source(source, Global_evm.program, EVM_PROGRAM_CAPACITY);
	evm_save_program_to_file(Global_evm.program, Global_evm.program_size, output_file_path);

	return 0;
}

