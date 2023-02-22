#ifndef EVM_H_
#define EVM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

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
#define LABELS_CAPACITY 1024
#define UNRESOLVED_JMP_CAPACITY 1024

typedef int64_t Word;

typedef enum {
	TRAP_OK = 0,
	TRAP_STACK_OVERFLOW,
	TRAP_STACK_UNDERFLOW,
	TRAP_ILLEGAL_INST,
	TRAP_ILLEGAL_INST_ACCESS,
	TRAP_ILLEGAL_OPERAND,
	TRAP_DIV_BY_ZERO,
} Trap;

const char *trap_as_cstr(Trap trap);

typedef enum {
	INST_NOP = 0,
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

typedef struct {
	Inst_Type type;
	Word operand;
} Inst;

const char *inst_type_as_cstr(Inst_Type type);

typedef struct {
	Word stack[EVM_STACK_CAPACITY];
	Word stack_size;

	Inst program[EVM_PROGRAM_CAPACITY];
	Word program_size;
	Word ip;

	uint8_t halt;
} EVM;

Trap evm_execute_inst(EVM *evm);
Trap evm_execute_program(EVM *evm, int limit);
void evm_dump_stack(FILE *stream, const EVM *evm);
void evm_push_inst(EVM *evm, Inst inst);
void evm_load_program_from_memory(EVM *evm, Inst *program, size_t program_size);
void evm_load_program_from_file(EVM *evm, const char *file_path);
void evm_save_program_to_file(const EVM *evm, const char *file_path);

typedef struct {
	size_t count;
	const char *data;
} String_View;

String_View cstr_as_sv(const char *cstr);
int sv_eq(String_View a, String_View b);
int sv_to_int(String_View sv);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_trim(String_View sv);
String_View sv_chop_by_delim(String_View *sv, char delim);
String_View sv_slurp_file(const char *file_path);

typedef struct {
	String_View name;
	Word addr;
} Label;

typedef struct {
	Word addr;
	String_View label;
} Defered_Operand;

typedef struct {
	Label labels[LABELS_CAPACITY];
	size_t labels_size;
	Defered_Operand defered_operands[UNRESOLVED_JMP_CAPACITY];
	size_t defered_operands_size;
} Label_Table;

Word lt_find_label_addr(const Label_Table *lt, String_View name);
void lt_push(Label_Table *lt, String_View name, Word addr);
void lt_push_defered_operand(Label_Table *lt, Word addr, String_View name);

void evm_transalte_source(String_View source, EVM *evm, Label_Table *lt);

#endif // EVM_H_

#ifdef EVM_IMPLEMENTATION

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

const char *inst_type_as_cstr(Inst_Type type) {
	switch (type) {
		case INST_NOP:
			return "INST_NOP";
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

Trap evm_execute_inst(EVM *evm) {
	if(evm->ip < 0 || evm->ip >= evm->program_size) return TRAP_ILLEGAL_INST_ACCESS;

	Inst inst = evm->program[evm->ip];

	switch (inst.type) {
		case INST_NOP:
			evm->ip += 1;
		break;

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

Trap evm_execute_program(EVM *evm, int limit) {
	while (limit != 0 && !evm->halt) {
		Trap trap = evm_execute_inst(evm);
		if (trap != TRAP_OK) {
			return trap;
		}

		if (limit > 0) --limit;
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

void evm_load_program_from_file(EVM *evm, const char *file_path) {
	FILE *f = fopen(file_path, "rb");
	if (f == NULL) {
		fprintf(stderr, "Could not open file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	long m = ftell(f);
	if (m < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	assert(m % sizeof(evm->program[0]) == 0);
	assert((size_t)m <= EVM_PROGRAM_CAPACITY * sizeof(evm->program[0]));

	if (fseek(f, 0, SEEK_SET) < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	evm->program_size = fread(evm->program, sizeof(evm->program[0]), m / sizeof(evm->program[0]), f);

	if (ferror(f)) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	fclose(f);
}

void evm_save_program_to_file(const EVM *evm, const char *file_path) {
	FILE *f = fopen(file_path, "wb");
	if (f == NULL) {
		fprintf(stderr, "Could not open file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	fwrite(evm->program, sizeof(evm->program[0]), evm->program_size, f);

	if (ferror(f)) {
		fprintf(stderr, "Could not write to file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	fclose(f);
}

String_View cstr_as_sv(const char *cstr) {
	return (String_View) {
		.count = strlen(cstr),
		.data = cstr,
	};
}

int sv_eq(String_View a, String_View b) {
	if (a.count != b.count) return 0;
	else return (memcmp(a.data, b.data, a.count) == 0);
}

int sv_to_int(String_View sv) {
	int result = 0;

	for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
		result = result * 10 + sv.data[i] - '0';
	}

	return result;
}

String_View sv_trim_left(String_View sv) {
	size_t i = 0;
	while (i < sv.count && isspace(sv.data[i])) {
		i += 1;
	}

	return (String_View) {
		.count = sv.count - i,
		.data = sv.data + i,
	};
}

String_View sv_trim_right(String_View sv) {
	size_t i = 0;
	while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
		i += 1;
	}

	return (String_View) {
		.count = sv.count - i,
		.data = sv.data,
	};
}

String_View sv_trim(String_View sv) {
	return sv_trim_right(sv_trim_left(sv));
}

String_View sv_chop_by_delim(String_View *sv, char delim) {
	size_t i = 0;
	while (i < sv->count && sv->data[i] != delim) {
		i += 1;
	}

	String_View result = {
		.count = i,
		.data = sv->data,
	};

	if (i < sv->count) {
		sv->count -= i + 1;
		sv->data += i + 1;
	} else {
		sv->count -= i;
		sv->data += i;
	}

	return result;
}

Word lt_find_label_addr(const Label_Table *lt, String_View name) {
	for (size_t i = 0; i < lt->labels_size; ++i) {
		if (sv_eq(lt->labels[i].name, name)) {
			return lt->labels[i].addr;
		}
	}

	fprintf(stderr, "ERROR: label '%.*s' does not exist\n", (int)name.count, name.data);
	exit(1);
}

void lt_push(Label_Table *lt, String_View name, Word addr) {
	assert(lt->labels_size < LABELS_CAPACITY);
	lt->labels[lt->labels_size++] = (Label) {
		.name = name,
		.addr = addr,
	};
}

void lt_push_defered_operand(Label_Table *lt, Word addr, String_View label) {
	assert(lt->defered_operands_size < UNRESOLVED_JMP_CAPACITY);
	lt->defered_operands[lt->defered_operands_size++] = (Defered_Operand) {
		.addr = addr,
		.label = label,
	};
}

void evm_transalte_source(String_View source, EVM *evm, Label_Table *lt) {
	evm->program_size = 0;

	while (source.count > 0) {
		assert(evm->program_size < EVM_PROGRAM_CAPACITY);
		String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
		if (line.count > 0 && *line.data != ';') {
			String_View inst_name = sv_chop_by_delim(&line, ' ');

			if (inst_name.count > 0 && inst_name.data[inst_name.count - 1] == ':') {
				String_View label = {
					.count = inst_name.count - 1,
					.data = inst_name.data,
				};
				lt_push(lt, label, evm->program_size);
				// We need to time forst so we support multiple spaces
				line = sv_trim(line);
				inst_name = sv_chop_by_delim(&line, ' ');
			}

			if (inst_name.count > 0) {
				String_View operand = sv_trim(sv_chop_by_delim(&line, ';'));

				if (sv_eq(inst_name, cstr_as_sv("nop"))) {
					evm_push_inst(evm, (Inst) { .type = INST_NOP, .operand = 0 });
				} else if (sv_eq(inst_name, cstr_as_sv("push"))) {
					evm_push_inst(evm, (Inst) { .type = INST_PUSH, .operand = sv_to_int(operand) });
				} else if (sv_eq(inst_name, cstr_as_sv("dup"))) {
					evm_push_inst(evm, (Inst) { .type = INST_DUP, .operand = sv_to_int(operand) });
				} else if (sv_eq(inst_name, cstr_as_sv("plus"))) {
					evm_push_inst(evm, (Inst) { .type = INST_PLUS, .operand = 0 });
				} else if (sv_eq(inst_name, cstr_as_sv("minus"))) {
					evm_push_inst(evm, (Inst) { .type = INST_MINUS, .operand = 0 });
				} else if (sv_eq(inst_name, cstr_as_sv("mult"))) {
					evm_push_inst(evm, (Inst) { .type = INST_MULT, .operand = 0 });
				} else if (sv_eq(inst_name, cstr_as_sv("div"))) {
					evm_push_inst(evm, (Inst) { .type = INST_DIV, .operand = 0 });
				} else if (sv_eq(inst_name, cstr_as_sv("jmp"))) {
					Word addr = 0;
					if (operand.count > 0 && isdigit(*operand.data)) {
						addr = sv_to_int(operand);
					} else {
						lt_push_defered_operand(lt, evm->program_size, operand);
					}
					evm_push_inst(evm, (Inst) { .type = INST_JMP, .operand = addr });
				} else if (sv_eq(inst_name, cstr_as_sv("jmp_if"))) {
					Word addr = 0;
					if (operand.count > 0 && isdigit(*operand.data)) {
						addr = sv_to_int(operand);
					} else {
						lt_push_defered_operand(lt, evm->program_size, operand);
					}
					evm_push_inst(evm, (Inst) { .type = INST_JMP, .operand = addr });
				} else if (sv_eq(inst_name, cstr_as_sv("eq"))) {
					evm_push_inst(evm, (Inst) { .type = INST_EQ, .operand = 0 });
				} else if (sv_eq(inst_name, cstr_as_sv("print_debug"))) {
					evm_push_inst(evm, (Inst) { .type = INST_PRINT_DEBUG, .operand = 0 });
				} else if (sv_eq(inst_name, cstr_as_sv("halt"))) {
					evm_push_inst(evm, (Inst) { .type = INST_HALT, .operand = 0 });
				} else {
					fprintf(stderr, "ERROR: unknown instruction '%.*s'\n", (int)inst_name.count, inst_name.data);
					exit(1);
				}
			}
		}
	}

	for (size_t i = 0; i < lt->defered_operands_size; ++i) {
		Word addr = lt_find_label_addr(lt, lt->defered_operands[i].label);
		evm->program[lt->defered_operands[i].addr].operand = addr;
	}
}

String_View sv_slurp_file(const char *file_path) {
	FILE *f = fopen(file_path, "r");

	if (f == NULL) {
		fprintf(stderr, "Could not open file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	long m = ftell(f);
	if (m < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	char *buffer = malloc(m);
	if (buffer == NULL) {
		fprintf(stderr, "Could not allocate memory for file: %s\n", strerror(errno));
		exit(1);
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	size_t n = fread(buffer, 1, m, f);
	if (ferror(f)) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	fclose(f);

	return (String_View) {
		.count = n,
		.data = buffer,
	};
}

#endif //EVM_IMPLEMENTATION
