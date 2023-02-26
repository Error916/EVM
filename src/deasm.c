#define EVM_IMPLEMENTATION
#include "./evm.h"

int main(int argc, char **argv) {
    	if (argc < 2) {
		fprintf(stderr, "Usage: ./deasm <input.evm>\n");
        	fprintf(stderr, "ERROR: no input is provided\n");
        	exit(1);
    	}

    	const char *input_file_path = argv[1];

	EVM evm = { 0 };

    	evm_load_program_from_file(&evm, input_file_path);

	for (Inst_Addr i = 0; i < evm.program_size; ++i) {
		switch (evm.program[i].type) {
			case INST_NOP:
				printf("nop\n");
				break;
			case INST_PUSH:
				printf("push %ld\n", evm.program[i].operand.as_i64);
				break;
			case INST_DUP:
				printf("dup %ld\n", evm.program[i].operand.as_i64);
				break;
			case INST_PLUS:
				printf("plus\n");
				break;
			case INST_MINUS:
				printf("minus\n");
				break;
			case INST_MULT:
				printf("mult\n");
				break;
			case INST_DIV:
				printf("div\n");
				break;
			case INST_JMP:
				printf("jmp %ld\n", evm.program[i].operand.as_i64);
				break;
			case INST_JMP_IF:
				printf("jmp_if %ld\n", evm.program[i].operand.as_i64);
				break;
			case INST_EQ:
				printf("eq\n");
				break;
			case INST_HALT:
				printf("halt\n");
				break;
			case INST_PRINT_DEBUG:
				printf("print_debug\n");
				break;
        	}
	}

	return 0;
}
