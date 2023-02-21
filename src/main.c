#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

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
#define ARRAY_SIZE(xs) sizeof(xs) / sizeof((xs)[0])

#define EVM_STACK_CAPACITY 1024
#define EVM_PROGRAM_CAPACITY 1024
#define EVM_EXEWCUTION_LIMIT 69

typedef enum {
	TRAP_OK = 0,
	TRAP_STACK_OVERFLOW,
	TRAP_STACK_UNDERFLOW,
	TRAP_ILLEGAL_INST,
	TRAP_ILLEGAL_INST_ACCESS,
	TRAP_ILLEGAL_OPERAND,
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
		case TRAP_ILLEGAL_INST_ACCESS:
			return "TRAP_ILLEGAL_INST_ACCESS";
		case TRAP_ILLEGAL_OPERAND:
			return "TRAP_ILLEGAL_OPERAND";
		case TRAP_DIV_BY_ZERO:
			return "TRAP_DIV_BY_ZERO";
		default:
			UNREACHABLE("NOT EXISTING TRAP");
	}
}

typedef int64_t Word;

typedef enum {
	INST_PUSH,
	INST_DUP,
	INST_PLUS,
	INST_MINUS,
	INST_MULT,
	INST_DIV,
	INST_JMP,
	INST_JMP_IF,
	INST_EQ,
	INST_HALT,
	INST_PRINT_DEBUG,
} Inst_Type;

const char *inst_type_as_cstr(Inst_Type type) {
	switch (type) {
		case INST_PUSH:
			return "INST_PUSH";
		case INST_DUP:
			return "INST_DUP";
		case INST_PLUS:
			return "INST_PLUS";
		case INST_MINUS:
			return "INST_MINUS";
		case INST_MULT:
			return "INST_MULT";
		case INST_DIV:
			return "INST_DIV";
		case INST_JMP:
			return "INST_JMP";
		case INST_JMP_IF:
			return "INST_JMP_IF";
		case INST_EQ:
			return "INST_EQ";
		case INST_HALT:
			return "INST_HALT";
		case INST_PRINT_DEBUG:
			return "INST_PRINT_DEBUG";
		default:
			UNREACHABLE("NOT EXISTING INST_TYPE");
	}
}

typedef struct {
	Inst_Type type;
	Word operand;
} Inst;

typedef struct {
	Word stack[EVM_STACK_CAPACITY];
	Word stack_size;

	Inst program[EVM_PROGRAM_CAPACITY];
	Word program_size;
	Word ip;

	uint8_t halt;
} EVM;

#define MAKE_INST_PUSH(value) {.type = INST_PUSH, .operand = (value),}
#define MAKE_INST_DUP(addr) {.type = INST_DUP, .operand = (addr),}
#define MAKE_INST_PLUS() {.type = INST_PLUS, .operand = 0,}
#define MAKE_INST_MINUS() {.type = INST_MINUS, .operand = 0,}
#define MAKE_INST_MULT() {.type = INST_MULT, .operand = 0,}
#define MAKE_INST_DIV() {.type = INST_DIV, .operand = 0,}
#define MAKE_INST_JMP(addr) {.type = INST_JMP, .operand = (addr),}
#define MAKE_INST_JMP_IF() {.type = INST_JMP_IF, .operand = (addr),}
#define MAKE_INST_EQ() {.type = INST_EQ, .operand = 0,}
#define MAKE_INST_HALT() {.type = INST_HALT, .operand = 0,}

Trap evm_execute_inst(EVM *evm) {
	if(evm->ip < 0 || evm->ip >= evm->program_size) return TRAP_ILLEGAL_INST_ACCESS;

	Inst inst = evm->program[evm->ip];

	switch (inst.type) {
		case INST_PUSH:
			if (evm->stack_size > EVM_STACK_CAPACITY) return TRAP_STACK_OVERFLOW;
			evm->stack[evm->stack_size++] = inst.operand;
			evm->ip += 1;
		break;

		case INST_DUP:
			if (evm->stack_size > EVM_STACK_CAPACITY) return TRAP_STACK_OVERFLOW;
			if (evm->stack_size - inst.operand <= 0) return TRAP_STACK_UNDERFLOW;
			if (inst.operand < 0) return TRAP_ILLEGAL_OPERAND;
			evm->stack[evm->stack_size] = evm->stack[evm->stack_size - 1 - inst.operand];
			evm->stack_size += 1;
			evm->ip += 1;
		break;

		case INST_PLUS:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2] += evm->stack[evm->stack_size - 1];
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_MINUS:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2] -= evm->stack[evm->stack_size - 1];
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_MULT:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2] *= evm->stack[evm->stack_size - 1];
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_DIV:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			if (evm->stack[evm->stack_size - 1] == 0) return TRAP_DIV_BY_ZERO;
			evm->stack[evm->stack_size - 2] /= evm->stack[evm->stack_size - 1];
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_JMP:
			evm->ip = inst.operand;
		break;

		case INST_JMP_IF:
			if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			if (evm->stack[evm->stack_size - 1]) {
				evm->stack_size -= 1;
				evm->ip = inst.operand;
			} else {
				evm->ip += 1;
			}
		break;

		case INST_EQ:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2] = (evm->stack[evm->stack_size - 1] == evm->stack[evm->stack_size - 2]);
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_HALT:
			evm->halt = 1;
		break;

		case INST_PRINT_DEBUG:
			if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			printf("%ld\n", evm->stack[evm->stack_size - 1]);
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		default:
			return TRAP_ILLEGAL_INST;
	}

	return TRAP_OK;
}


void evm_dump_stack(FILE *stream, const EVM *evm) {
	fprintf(stream, "Stack:\n");
	if(evm->stack_size  > 0) {
		for (Word i = 0; i < evm->stack_size; ++i) {
			fprintf(stream, "\t%ld\n", evm->stack[i]);
		}
	} else {
		fprintf(stream, "\t[empty]\n");
	}
}

void evm_push_inst(EVM *evm, Inst inst) {
	assert(evm->program_size < EVM_PROGRAM_CAPACITY);
	evm->program[evm->program_size++] = inst;
}

void evm_load_program_from_memory(EVM *evm, Inst *program, size_t program_size) {
	assert(program_size < EVM_PROGRAM_CAPACITY);
	for (size_t i = 0; i < program_size; ++i) {
		evm_push_inst(evm, *(program +i));
	}
}

EVM Global_evm = {0};
Inst program[] = {
	MAKE_INST_PUSH(0),	// 0
	MAKE_INST_PUSH(1),	// 1
	MAKE_INST_DUP(1),	// 2
	MAKE_INST_DUP(1),	// 3
	MAKE_INST_PLUS(),	// 4
	MAKE_INST_JMP(2),	// 5
};

int main(int argc, char **argv) {
	UNUSED(argc);
	UNUSED(argv);

	evm_load_program_from_memory(&Global_evm, program, ARRAY_SIZE(program));
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
