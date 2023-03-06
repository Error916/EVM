#ifndef EVM_H_
#define EVM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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

#define EVM_STACK_CAPACITY 1024
#define EVM_PROGRAM_CAPACITY 1024
#define EVM_NATIVES_CAPACITY 1024
#define EVM_MEMORY_CAPACITY (640 * 1000)

#define EASM_LABELS_CAPACITY 1024
#define EASM_DEFERRED_OPERANDS_CAPACITY 1024
#define EASM_COMMENT_CHAR ';'
#define EASM_PP_CHAR '#'
#define EASM_MAX_INCLUDE_LEVEL 64
#define EASM_ARENA_CAPACITY (1000 * 1000 * 1000)

typedef struct {
	size_t count;
	const char *data;
} String_View;

#define SV_FORMAT(sv) (int)sv.count, sv.data

String_View cstr_as_sv(const char *cstr);
bool sv_eq(String_View a, String_View b);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_trim(String_View sv);
String_View sv_chop_by_delim(String_View *sv, char delim);

typedef uint64_t Inst_Addr;
typedef uint64_t Memory_Addr;

typedef union {
	uint64_t as_u64;
	int64_t as_i64;
	double as_f64;
	void *as_ptr;
} Word;
static_assert(sizeof(Word) == 8, "The BM's Word is expected to be 64 bits");

typedef enum {
	TRAP_OK = 0,
	TRAP_STACK_OVERFLOW,
	TRAP_STACK_UNDERFLOW,
	TRAP_ILLEGAL_INST,
	TRAP_ILLEGAL_INST_ACCESS,
	TRAP_ILLEGAL_MEMORY_ACCESS,
	TRAP_ILLEGAL_OPERAND,
	TRAP_DIV_BY_ZERO,
} Trap;

const char *trap_as_cstr(Trap trap);

typedef enum {
	INST_NOP = 0,
	INST_PUSH,
	INST_DROP,
	INST_DUP,
	INST_SWAP,
	INST_PLUSI,
	INST_MINUSI,
	INST_MULTI,
	INST_DIVI,
	INST_PLUSF,
	INST_MINUSF,
	INST_MULTF,
	INST_DIVF,
	INST_JMP,
	INST_JMP_IF,
	INST_RET,
	INST_CALL,
	INST_NATIVE,
	INST_EQ,
	INST_HALT,
	INST_NOT,
    	INST_GEF,
	INST_ANDB,
    	INST_ORB,
   	INST_XOR,
  	INST_SHR,
    	INST_SHL,
    	INST_NOTB,
	INST_READ8,
	INST_READ16,
	INST_READ32,
	INST_READ64,
	INST_WRITE8,
	INST_WRITE16,
	INST_WRITE32,
	INST_WRITE64,
	EASM_NUMBER_OF_INSTS,
} Inst_Type;

typedef struct {
	Inst_Type type;
	Word operand;
} Inst;

const char *inst_name(Inst_Type type);
int inst_has_operand(Inst_Type type);
bool inst_by_name(String_View name, Inst_Type *type);

typedef struct EVM EVM;

typedef Trap (*Evm_Native)(EVM *);

struct EVM {
	Word stack[EVM_STACK_CAPACITY];
	uint64_t stack_size;

	Inst program[EVM_PROGRAM_CAPACITY];
	uint64_t program_size;
	Inst_Addr ip;

	Evm_Native natives[EVM_NATIVES_CAPACITY];
	uint64_t natives_size;

	uint8_t memory[EVM_MEMORY_CAPACITY];

	bool halt;
};

Trap evm_execute_inst(EVM *evm);
Trap evm_execute_program(EVM *evm, int limit);
void evm_push_native(EVM *evm, Evm_Native native);
void evm_dump_stack(FILE *stream, const EVM *evm);
void evm_dump_memory(FILE *stream, const EVM *evm);
void evm_push_inst(EVM *evm, Inst inst);
void evm_load_program_from_memory(EVM *evm, Inst *program, size_t program_size);
void evm_load_program_from_file(EVM *evm, const char *file_path);
void evm_save_program_to_file(const EVM *evm, const char *file_path);

typedef struct {
	String_View name;
	Word word;
} Label;

typedef struct {
	Inst_Addr addr;
	String_View label;
} Deferred_Operand;

typedef struct {
	Label labels[EASM_LABELS_CAPACITY];
	size_t labels_size;
	Deferred_Operand deferred_operands[EASM_DEFERRED_OPERANDS_CAPACITY];
	size_t deferred_operands_size;
	unsigned char arena[EASM_ARENA_CAPACITY];
	size_t arena_size;
} EASM;

void *easm_alloc(EASM *easm, size_t size);
String_View easm_slurp_file(EASM *easm, String_View file_path);
bool easm_resolve_label(const EASM *easm, String_View name, Word *output);
bool easm_bind_label(EASM *easm, String_View name, Word word);
void easm_push_deferred_operand(EASM *easm, Inst_Addr addr, String_View name);
void easm_translate_source(EVM *evm, EASM *easm, String_View input_file_path, size_t level);
bool easm_number_literal_as_word(EASM *easm, String_View sv, Word *output);

#endif // EVM_H_

#ifdef EVM_IMPLEMENTATION

const char *trap_as_cstr(Trap trap) {
	switch (trap) {
		case TRAP_OK:				return "TRAP_OK";
		case TRAP_STACK_OVERFLOW:		return "TRAP_STACK_OVERFLOW";
		case TRAP_STACK_UNDERFLOW:		return "TRAP_STACK_UNDERFLOW";
		case TRAP_ILLEGAL_INST:			return "TRAP_ILLEGAL_INST";
		case TRAP_ILLEGAL_INST_ACCESS: 		return "TRAP_ILLEGAL_INST_ACCESS";
		case TRAP_ILLEGAL_MEMORY_ACCESS: 	return "TRAP_ILLEGAL_MEMORY_ACCESS";
		case TRAP_ILLEGAL_OPERAND:		return "TRAP_ILLEGAL_OPERAND";
		case TRAP_DIV_BY_ZERO:			return "TRAP_DIV_BY_ZERO";
		default: UNREACHABLE("NOT EXISTING TRAP");
	}
}

const char *inst_name(Inst_Type type) {
	switch (type) {
		case INST_NOP:         	return "nop";
		case INST_PUSH:        	return "push";
		case INST_DROP:		return "drop";
		case INST_DUP:         	return "dup";
		case INST_SWAP:		return "swap";
		case INST_PLUSI:       	return "plusi";
		case INST_MINUSI:      	return "minusi";
		case INST_MULTI:       	return "multi";
		case INST_DIVI:        	return "divi";
		case INST_PLUSF:       	return "plusf";
		case INST_MINUSF:      	return "minusf";
		case INST_MULTF:       	return "multf";
		case INST_DIVF:        	return "divf";
		case INST_JMP:         	return "jmp";
		case INST_JMP_IF:      	return "jmp_if";
		case INST_RET:		return "ret";
		case INST_CALL:		return "call";
		case INST_NATIVE:	return "native";
		case INST_EQ:          	return "eq";
		case INST_NOT:		return "not";
		case INST_GEF:		return "gef";
		case INST_HALT:        	return "halt";
		case INST_ANDB:		return "andb";
		case INST_ORB:		return "orb";
		case INST_XOR:		return "xor";
		case INST_SHR:		return "shr";
		case INST_SHL:		return "shl";
		case INST_NOTB:		return "notb";
		case INST_READ8:	return "read8";
		case INST_READ16:	return "read16";
		case INST_READ32:	return "read32";
		case INST_READ64:	return "read64";
		case INST_WRITE8:	return "write8";
		case INST_WRITE16:	return "write16";
		case INST_WRITE32:	return "write32";
		case INST_WRITE64:	return "write64";
		case EASM_NUMBER_OF_INSTS:
		default: UNREACHABLE("NOT EXISTING INST_TYPE");
	}
}

int inst_has_operand(Inst_Type type) {
	switch (type) {
		case INST_NOP:         	return 0;
		case INST_PUSH:        	return 1;
		case INST_DROP:		return 0;
		case INST_DUP:         	return 1;
		case INST_SWAP:		return 1;
		case INST_PLUSI:       	return 0;
		case INST_MINUSI:      	return 0;
		case INST_MULTI:       	return 0;
		case INST_DIVI:        	return 0;
		case INST_PLUSF:       	return 0;
		case INST_MINUSF:      	return 0;
		case INST_MULTF:       	return 0;
		case INST_DIVF:        	return 0;
		case INST_JMP:         	return 1;
		case INST_JMP_IF:      	return 1;
		case INST_RET:		return 0;
		case INST_CALL:		return 1;
		case INST_NATIVE:	return 1;
		case INST_EQ:          	return 0;
		case INST_NOT:		return 0;
		case INST_GEF:		return 0;
		case INST_HALT:        	return 0;
		case INST_ANDB:   	return 0;
		case INST_ORB:    	return 0;
		case INST_XOR:    	return 0;
		case INST_SHR:    	return 0;
		case INST_SHL:    	return 0;
		case INST_NOTB:   	return 0;
		case INST_READ8:	return 0;
		case INST_READ16:	return 0;
		case INST_READ32:	return 0;
		case INST_READ64:	return 0;
		case INST_WRITE8:	return 0;
		case INST_WRITE16:	return 0;
		case INST_WRITE32:	return 0;
		case INST_WRITE64:	return 0;
		case EASM_NUMBER_OF_INSTS:
		default: UNREACHABLE("NOT EXISTING INST_TYPE");
	}
}

bool inst_by_name(String_View name, Inst_Type *type) {
	for (Inst_Type t = (Inst_Type) 0; t < EASM_NUMBER_OF_INSTS; ++t) {
		if (sv_eq(cstr_as_sv(inst_name(t)), name)) {
			*type = t;
			return true;
		}
	}
	return false;
}

Trap evm_execute_inst(EVM *evm) {
	if(evm->ip >= evm->program_size) return TRAP_ILLEGAL_INST_ACCESS;

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

		case INST_DROP:
			if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_DUP:
			if (evm->stack_size > EVM_STACK_CAPACITY) return TRAP_STACK_OVERFLOW;
			if (evm->stack_size - inst.operand.as_u64 <= 0) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size] = evm->stack[evm->stack_size - 1 - inst.operand.as_u64];
			evm->stack_size += 1;
			evm->ip += 1;
		break;

		case INST_PLUSI:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_u64 += evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_MINUSI:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_u64 -= evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_MULTI:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_u64 *= evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_DIVI:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			if (evm->stack[evm->stack_size - 1].as_u64 == 0) return TRAP_DIV_BY_ZERO;
			evm->stack[evm->stack_size - 2].as_u64 /= evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_PLUSF:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_f64 += evm->stack[evm->stack_size - 1].as_f64;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_MINUSF:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_f64 -= evm->stack[evm->stack_size - 1].as_f64;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_MULTF:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_f64 *= evm->stack[evm->stack_size - 1].as_f64;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_DIVF:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_f64 /= evm->stack[evm->stack_size - 1].as_f64;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_JMP:
			evm->ip = inst.operand.as_u64;
		break;

		case INST_JMP_IF:
			if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			if (evm->stack[evm->stack_size - 1].as_u64) {
				evm->ip = inst.operand.as_u64;
			} else {
				evm->ip += 1;
			}
			evm->stack_size -= 1;
		break;

		case INST_RET:
			if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			evm->ip = evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 1;
		break;

		case INST_CALL:
			if (evm->stack_size > EVM_STACK_CAPACITY) return TRAP_STACK_OVERFLOW;
			evm->stack[evm->stack_size++].as_u64 = evm->ip + 1;
			evm->ip = inst.operand.as_u64;
		break;

		case INST_NATIVE:
			if (inst.operand.as_u64 > evm->natives_size) return TRAP_ILLEGAL_OPERAND;
			const Trap trap = evm->natives[inst.operand.as_u64](evm);
			if (trap != TRAP_OK) return trap;
			evm->ip += 1;
		break;

		case INST_NOT:
			if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 1].as_u64 = !evm->stack[evm->stack_size - 1].as_u64;
			evm->ip += 1;
		break;

		case INST_EQ:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_u64 = (evm->stack[evm->stack_size - 1].as_u64 == evm->stack[evm->stack_size - 2].as_u64);
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_GEF:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 2].as_u64 = (evm->stack[evm->stack_size - 1].as_u64 >= evm->stack[evm->stack_size - 2].as_u64);
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_HALT:
			evm->halt = true;
		break;

		case INST_SWAP:
			if (inst.operand.as_u64 >= evm->stack_size) return TRAP_STACK_UNDERFLOW;
			const uint64_t a = evm->stack_size - 1;
			const uint64_t b = evm->stack_size - 1 - inst.operand.as_u64;
			Word t = evm->stack[a];
			evm->stack[a] = evm->stack[b];
			evm->stack[b] = t;
			evm->ip += 1;
		break;

		case INST_ANDB:
		        if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
        		evm->stack[evm->stack_size - 2].as_u64 = evm->stack[evm->stack_size - 2].as_u64 & evm->stack[evm->stack_size - 1].as_u64;
        		evm->stack_size -= 1;
        		evm->ip += 1;
        	break;

    		case INST_ORB:
        		if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
        		evm->stack[evm->stack_size - 2].as_u64 = evm->stack[evm->stack_size - 2].as_u64 | evm->stack[evm->stack_size - 1].as_u64;
        		evm->stack_size -= 1;
        		evm->ip += 1;
        	break;

    		case INST_XOR:
        		if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
        		evm->stack[evm->stack_size - 2].as_u64 = evm->stack[evm->stack_size - 2].as_u64 ^ evm->stack[evm->stack_size - 1].as_u64;
        		evm->stack_size -= 1;
        		evm->ip += 1;
        	break;

    		case INST_SHR:
			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
        		evm->stack[evm->stack_size - 2].as_u64 = evm->stack[evm->stack_size - 2].as_u64 >> evm->stack[evm->stack_size - 1].as_u64;
        		evm->stack_size -= 1;
        		evm->ip += 1;
        	break;

    		case INST_SHL:
       			if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
        		evm->stack[evm->stack_size - 2].as_u64 = evm->stack[evm->stack_size - 2].as_u64 << evm->stack[evm->stack_size - 1].as_u64;
        		evm->stack_size -= 1;
       			evm->ip += 1;
        	break;

    		case INST_NOTB:
        		if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
        		evm->stack[evm->stack_size - 1].as_u64 = ~evm->stack[evm->stack_size - 1].as_u64;
        		evm->ip += 1;
        	break;

		case INST_READ8: {
        		if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 1].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY) return TRAP_ILLEGAL_MEMORY_ACCESS;
			evm->stack[evm->stack_size - 1].as_u64 = *(uint8_t*)&evm->memory[addr];
        		evm->ip += 1;
		} break;

		case INST_READ16: {
        		if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 1].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 1) return TRAP_ILLEGAL_MEMORY_ACCESS;
			evm->stack[evm->stack_size - 1].as_u64 = *(uint16_t*)&evm->memory[addr];
        		evm->ip += 1;
		} break;

		case INST_READ32: {
        		if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 1].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 3) return TRAP_ILLEGAL_MEMORY_ACCESS;
			evm->stack[evm->stack_size - 1].as_u64 = *(uint32_t*)&evm->memory[addr];
        		evm->ip += 1;
		} break;

		case INST_READ64: {
        		if (evm->stack_size < 1) return TRAP_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 1].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 7) return TRAP_ILLEGAL_MEMORY_ACCESS;
			evm->stack[evm->stack_size - 1].as_u64 = *(uint64_t*)&evm->memory[addr];
        		evm->ip += 1;
		} break;

		case INST_WRITE8: {
        		if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY) return TRAP_ILLEGAL_MEMORY_ACCESS;
			*(uint8_t*)&evm->memory[addr] = (uint8_t)evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 2;
        		evm->ip += 1;
		} break;

		case INST_WRITE16: {
        		if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 1) return TRAP_ILLEGAL_MEMORY_ACCESS;
			*(uint16_t*)&evm->memory[addr] = (uint16_t)evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 2;
        		evm->ip += 1;
		} break;

		case INST_WRITE32: {
        		if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 3) return TRAP_ILLEGAL_MEMORY_ACCESS;
			*(uint32_t*)&evm->memory[addr] = (uint32_t)evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 2;
        		evm->ip += 1;
		} break;

		case INST_WRITE64: {
        		if (evm->stack_size < 2) return TRAP_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 7) return TRAP_ILLEGAL_MEMORY_ACCESS;
			*(uint64_t*)&evm->memory[addr] = (uint64_t)evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 2;
        		evm->ip += 1;
		} break;

		case EASM_NUMBER_OF_INSTS:
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

void evm_push_native(EVM *evm, Evm_Native native) {
	assert(evm->natives_size < EVM_NATIVES_CAPACITY);
	evm->natives[evm->natives_size++] = native;
}

void evm_dump_stack(FILE *stream, const EVM *evm) {
	fprintf(stream, "Stack:\n");
	if(evm->stack_size  > 0) {
		for (Inst_Addr i = 0; i < evm->stack_size; ++i) {
			fprintf(stream, "  u64: %lu, i64: %ld, f64: %lf, ptr: %p\n",
                    		evm->stack[i].as_u64,
                    		evm->stack[i].as_i64,
                    		evm->stack[i].as_f64,
                    		evm->stack[i].as_ptr);
		}
	} else {
		fprintf(stream, "\t[empty]\n");
	}
}

void evm_dump_memory(FILE *stream, const EVM *evm) {
	fprintf(stream, "Memory:\n");
	for (size_t i = 0; i < EVM_MEMORY_CAPACITY; ++i) {
		fprintf(stream, "%02X ", evm->memory[i]);
	}
	fprintf(stream, "\n");
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
		fprintf(stderr, "Could not set position at end of file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	long m = ftell(f);
	if (m < 0) {
		fprintf(stderr, "Could not determinate length of file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	assert((size_t)m % sizeof(evm->program[0]) == 0);
	assert((size_t)m <= EVM_PROGRAM_CAPACITY * sizeof(evm->program[0]));

	if (fseek(f, 0, SEEK_SET) < 0) {
		fprintf(stderr, "Could not rewind file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	evm->program_size = fread(evm->program, sizeof(evm->program[0]), (size_t)m / sizeof(evm->program[0]), f);

	if (ferror(f)) {
		fprintf(stderr, "Could not consume file %s: %s\n", file_path, strerror(errno));
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

bool sv_eq(String_View a, String_View b) {
	if (a.count != b.count) return false;
	else return (memcmp(a.data, b.data, a.count) == 0);
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

// TODO: remove both spaces and tabs
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

void *easm_alloc(EASM *easm, size_t size) {
	assert(easm->arena_size + size <= EASM_ARENA_CAPACITY);

	void *result = easm->arena + easm->arena_size;
	easm->arena_size += size;
	return result;
}

bool easm_resolve_label(const EASM *easm, String_View name, Word *output) {
	for (size_t i = 0; i < easm->labels_size; ++i) {
		if (sv_eq(easm->labels[i].name, name)) {
			*output = easm->labels[i].word;
			return true;
		}
	}

	return false;
}

bool easm_bind_label(EASM *easm, String_View name, Word word) {
	assert(easm->labels_size < EASM_LABELS_CAPACITY);
	Word ignore = { 0 };
	if (easm_resolve_label(easm, name, &ignore)) return false;
	easm->labels[easm->labels_size++] = (Label) { .name = name, .word = word };
	return true;
}

void easm_push_deferred_operand(EASM *easm, Inst_Addr addr, String_View label) {
	assert(easm->deferred_operands_size < EASM_DEFERRED_OPERANDS_CAPACITY);
	easm->deferred_operands[easm->deferred_operands_size++] = (Deferred_Operand) {
		.addr = addr,
		.label = label,
	};
}

void easm_translate_source(EVM *evm, EASM *easm, String_View input_file_path, size_t level) {
	String_View original_source = easm_slurp_file(easm, input_file_path);
	String_View source = original_source;
	evm->program_size = 0;
	size_t line_number = 0;

	// First pass
	while (source.count > 0) {
		assert(evm->program_size < EVM_PROGRAM_CAPACITY);
		String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
		line_number += 1;
		if (line.count > 0 && *line.data != EASM_COMMENT_CHAR) {
			String_View token = sv_trim(sv_chop_by_delim(&line, ' '));

			// Pre-processor
			if (token.count > 0 && token.data[0] == EASM_PP_CHAR) {
				token.count -= 1;
				token.data += 1;
				if (sv_eq(token, cstr_as_sv("label"))) {
					line = sv_trim(line);
					String_View label = sv_chop_by_delim(&line, ' ');
					if (label.count > 0) {
						line = sv_trim(line);
						String_View value = sv_chop_by_delim(&line, ' ');
						Word word = { 0 };
						if (!easm_number_literal_as_word(easm, value, &word)) {
							fprintf(stderr, "ERROR: unknown pre-processor directive '%.*s' on line %lu\n", SV_FORMAT(value), line_number);
							exit(1);
						}

						if (!easm_bind_label(easm, label, word)) {
							fprintf(stderr, "ERROR: label '%.*s' is allready define\n", SV_FORMAT(label));
							exit(1);
						}
					} else {
						fprintf(stderr, "ERROR: label name in not provided on line %lu\n", line_number);
						exit(1);
					}
				} else if (sv_eq(token, cstr_as_sv("include"))) {
					line = sv_trim(line);
					if (line.count > 0) {
						if (line.data[0] == '"' && line.data[line.count - 1] == '"') {
							line.count -= 2;
							line.data += 1;

							if (level + 1 >= EASM_MAX_INCLUDE_LEVEL) {
								fprintf(stderr, "ERROR: exceeded maximum include level\n");
								exit(1);
							}

							easm_translate_source(evm, easm, line, level + 1);
						} else {
							fprintf(stderr, "ERROR: path must be surrounded by quotation marks on line %lu\n", line_number);
							exit(1);
						}
					} else {
						fprintf(stderr, "ERROR: include file path is not provided on line %lu\n", line_number);
						exit(1);
					}
			 	} else {
					fprintf(stderr, "ERROR: unknown pre-processor directive '%.*s' on line %lu\n", SV_FORMAT(token), line_number);
					exit(1);
				}
			} else {
				// Label
				if (token.count > 0 && token.data[token.count - 1] == ':') {
					String_View label = {
						.count = token.count - 1,
						.data = token.data,
					};
					if (!easm_bind_label(easm, label, (Word) { .as_u64 = evm->program_size })) {
						fprintf(stderr, "ERROR: label '%.*s' on line %lu is allready define\n", SV_FORMAT(label), line_number);
						exit(1);
					}
					token = sv_trim(sv_chop_by_delim(&line, ' '));
				}

				// Instraction
				if (token.count > 0) {
					String_View operand = sv_trim(sv_chop_by_delim(&line, EASM_COMMENT_CHAR));

					Inst_Type inst_type = INST_NOP;
					if (inst_by_name(token, &inst_type)) {
						evm->program[evm->program_size].type = inst_type;
						if (inst_has_operand(inst_type)) {
							if (operand.count == 0) {
								fprintf(stderr, "ERROR: instruction '%.*s' requires an operand on line %lu\n", SV_FORMAT(token), line_number);
								exit(1);
							}
							if (!easm_number_literal_as_word(easm, operand, &evm->program[evm->program_size].operand)) {
								easm_push_deferred_operand(easm, evm->program_size, operand);
							}
						}
						evm->program_size += 1;
					} else {
						fprintf(stderr, "ERROR: unknown instruction '%.*s' on line %lu\n", SV_FORMAT(token), line_number);
						exit(1);
					}
				}
			}
		}
	}

	// Second pass
	for (size_t i = 0; i < easm->deferred_operands_size; ++i) {
		String_View label = easm->deferred_operands[i].label;
		if (!easm_resolve_label(easm, label, &evm->program[easm->deferred_operands[i].addr].operand)) {
			fprintf(stderr, "ERROR: unknown label '%.*s'\n", SV_FORMAT(label));
			exit(1);
		}
	}
}

bool easm_number_literal_as_word(EASM *easm, String_View sv, Word *output) {
	char *cstr = easm_alloc(easm, sv.count + 1);
	memcpy(cstr, sv.data, sv.count);
	cstr[sv.count] = '\0';

	char *endptr = 0;
	Word result = { 0 };
	result.as_u64 = strtoull(cstr, &endptr, 10);
	if ((size_t) (endptr - cstr) != sv.count) {
		result.as_f64 = strtod(cstr, &endptr);
		if ((size_t) (endptr - cstr) != sv.count) return false;
	}

	*output = result;
	return true;
}

String_View easm_slurp_file(EASM *easm, String_View file_path) {
	char *file_path_cstr = easm_alloc(easm, file_path.count + 1);
	if (file_path_cstr == NULL) {
		fprintf(stderr, "Could not allocate memory for file path: %s\n", strerror(errno));
		exit(1);
	}
	memcpy(file_path_cstr, file_path.data, file_path.count);
	file_path_cstr[file_path.count] = '\0';

	FILE *f = fopen(file_path_cstr, "r");

	if (f == NULL) {
		fprintf(stderr, "Could not open file %s: %s\n", file_path_cstr, strerror(errno));
		exit(1);
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path_cstr, strerror(errno));
		exit(1);
	}

	long m = ftell(f);
	if (m < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path_cstr, strerror(errno));
		exit(1);
	}

	char *buffer = easm_alloc(easm, (size_t)m);
	if (buffer == NULL) {
		fprintf(stderr, "Could not allocate memory for file: %s\n", strerror(errno));
		exit(1);
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path_cstr, strerror(errno));
		exit(1);
	}

	size_t n = fread(buffer, 1, (size_t)m, f);
	if (ferror(f)) {
		fprintf(stderr, "Could not read file %s: %s\n", file_path_cstr, strerror(errno));
		exit(1);
	}

	fclose(f);

	return (String_View) {
		.count = n,
		.data = buffer,
	};
}

#endif //EVM_IMPLEMENTATION
