#define EVM_IMPLEMENTATION
#include "./evm.h"

static Trap evm_alloc(EVM *evm) {
	if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;

	evm->stack[evm->stack_size - 1].as_ptr = malloc(evm->stack[evm->stack_size].as_u64);

	return TRAP_OK;
}

static Trap evm_free(EVM *evm) {
	if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;

	free(evm->stack[evm->stack_size].as_ptr);
	evm->stack_size -= 1;

	return TRAP_OK;
}

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

	evm_push_native(&evm, evm_alloc);
	evm_push_native(&evm, evm_free);

	Trap trap = evm_execute_program(&evm , limit);
	evm_dump_stack(stdout, &evm);

	if (trap != TRAP_OK) {
		fprintf(stderr, "Trap activated: %s\n", trap_as_cstr(trap));
		return 1;
	}

	return 0;
}
