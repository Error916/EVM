#define EVM_IMPLEMENTATION
#include "./evm.h"

int main(int argc, char **argv) {
    	if (argc < 2) {
		fprintf(stderr, "Usage: ./deasm <input.evm>\n");
        	fprintf(stderr, "ERROR: no input is provided\n");
        	exit(1);
    	}

    	const char *input_file_path = argv[1];

	// NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
	static EVM evm = { 0 };

    	evm_load_program_from_file(&evm, input_file_path);

	for (Inst_Addr i = 0; i < evm.program_size; ++i) {
		printf("%s", inst_name(evm.program[i].type));
        	if (inst_has_operand(evm.program[i].type)) {
            		printf(" %ld", evm.program[i].operand.as_i64);
        	}
		printf("\n");
	}

	return 0;
}
