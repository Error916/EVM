#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define UNUSED(x) (void)(x)
#define UNIMPLEMENTED(message) \
    do { \
        fprintf(stderr, "%s:%d: UNIMPLEMENTED: %s\n", __FILE__, __LINE__, message); \
        exit(1); \
    } while (0)
#define UNREACHABLE(message) \
    do { \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); \
        exit(1); \
    } while (0)

#define EVM_STACK_CAPACITY 1024

typedef enum {
	TRAP_OK = 0,
	TRAP_STACK_OVERFLOW,
	TRAP_STACK_UNDERFLOW,
	TRAP_ILLEGAL_INST,
	TRAP_DIV_BY_ZERO,
} Trap;

const char *trap_as_cstr(Trap trap) {
	switch (trap) {
		case TRAP_OK:
			return "TRAP_OK";
		case TRAP_STACK_OVERFLOW:
			return "TRAP_STACK_OVERFLOW";
		case TRAP_STACK_UNDERFLOW:
			return "TRAP_STACK_UNDERFLOW";
		case TRAP_ILLEGAL_INST:
			return "TRAP_ILLEGAL_INST";
		case 	TRAP_DIV_BY_ZERO:
			return "TRAP_DIV_BY_ZERO";
		default:
			UNREACHABLE("NOT EXISTING TRAP");
	}
}

typedef int64_t Word;

typedef struct {
	Word stack[EVM_STACK_CAPACITY];
	size_t stack_size;
} EVM;

typedef enum {
	INST_PUSH,
	INST_PLUS,
	INST_MINUS,
	INST_MULT,
	INST_DIV,
} Inst_Type;

const char *inst_type_as_cstr(Inst_Type type) {
	switch (type) {
		case INST_PUSH:
			return "INST_PUSH";
		case INST_PLUS:
			return "INST_PLUS";
		case INST_MINUS:
			return "INST_MINUS";
		case INST_MULT:
			return "INST_MULT";
		case INST_DIV:
			return "INST_DIV";
		default:
			UNREACHABLE("NOT EXISTING INST_TYPE");
	}
}

typedef struct {
	Inst_Type type;
	Word operand;
} Inst;

#define MAKE_INST_PUSH(value) {.type = INST_PUSH, .operand = (value),}
#define MAKE_INST_PLUS() {.type = INST_PLUS, .operand = 0,}
#define MAKE_INST_MINUS() {.type = INST_MINUS, .operand = 0,}
#define MAKE_INST_MULT() {.type = INST_MULT, .operand = 0,}
#define MAKE_INST_DIV() {.type = INST_DIV, .operand = 0,}

Trap evm_execute_inst(EVM *evm, Inst inst) {
	switch (inst.type) {
		case INST_PUSH:
			if (evm->stack_size > EVM_STACK_CAPACITY) return TRAP_STACK_OVERFLOW;
			evm->stack[evm->stack_size++] = inst.operand;
		break;

		case INST_PLUS:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2] += evm->stack[evm->stack_size - 1];
			evm->stack_size -= 1;
		break;

		case INST_MINUS:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2] -= evm->stack[evm->stack_size - 1];
			evm->stack_size -= 1;
		break;

		case INST_MULT:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2] *= evm->stack[evm->stack_size - 1];
			evm->stack_size -= 1;
		break;

		case INST_DIV:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			if (evm->stack[evm->stack_size - 1] == 0) return TRAP_DIV_BY_ZERO;
			evm->stack[evm->stack_size - 2] /= evm->stack[evm->stack_size - 1];
			evm->stack_size -= 1;
		break;

		default:
			return TRAP_ILLEGAL_INST;
	}

	return TRAP_OK;
}


void evm_dump(FILE *stream, const EVM *evm) {
	fprintf(stream, "Stack:\n");
	if(evm->stack_size  > 0) {
		for (size_t i = 0; i < evm->stack_size; ++i) {
			fprintf(stream, "\t%ld\n", evm->stack[i]);
		}
	} else {
		fprintf(stream, "\t[empty]\n");
	}
}

#define ARRAY_SIZE(xs) sizeof(xs) / sizeof((xs)[0])
EVM Global_evm = {0};
const Inst program[] = {
	MAKE_INST_PUSH(69),
	MAKE_INST_PUSH(420),
	MAKE_INST_PLUS(),
	MAKE_INST_PUSH(42),
	MAKE_INST_MINUS(),
	MAKE_INST_PUSH(2),
	MAKE_INST_MULT(),
	MAKE_INST_PUSH(4),
	MAKE_INST_DIV(),
};

int main(int argc, char **argv) {
	UNUSED(argc);
	UNUSED(argv);

	evm_dump(stdout, &Global_evm);
	for (size_t i = 0; i < ARRAY_SIZE(program); ++i) {
		printf("%s\n", inst_type_as_cstr(program[i].type));
		Trap trap = evm_execute_inst(&Global_evm, program[i]);
		evm_dump(stdout, &Global_evm);
		if (trap != TRAP_OK) {
			fprintf(stderr, "Trap activated: %s\n", trap_as_cstr(trap));
			exit(1);
		}
	}

	return 0;
}
