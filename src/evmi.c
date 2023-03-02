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

static Trap evm_print_u64(EVM *evm) {
	if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;

	printf("%lu\n", evm->stack[evm->stack_size - 1].as_u64);
	evm->stack_size -= 1;

	return TRAP_OK;
}

static Trap evm_print_i64(EVM *evm) {
	if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;

	printf("%ld\n", evm->stack[evm->stack_size - 1].as_i64);
	evm->stack_size -= 1;

	return TRAP_OK;
}

static Trap evm_print_f64(EVM *evm) {
	if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;

	printf("%lf\n", evm->stack[evm->stack_size - 1].as_f64);
	evm->stack_size -= 1;

	return TRAP_OK;
}

static Trap evm_print_ptr(EVM *evm) {
	if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;

	printf("%p\n", evm->stack[evm->stack_size - 1].as_ptr);
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

	// TODO: mechanism to load native function more granuraly
	evm_push_native(&evm, evm_alloc);	// 0
	evm_push_native(&evm, evm_free);	// 1
	evm_push_native(&evm, evm_print_u64);	// 2
	evm_push_native(&evm, evm_print_i64);	// 3
	evm_push_native(&evm, evm_print_f64);	// 4
	evm_push_native(&evm, evm_print_ptr);	// 5

	Trap trap = evm_execute_program(&evm , limit);

	if (trap != TRAP_OK) {
		fprintf(stderr, "Trap activated: %s\n", trap_as_cstr(trap));
		return 1;
	}

	return 0;
}
