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

// NOTE: Stolen from https://stackoverflow.com/a/3312896
#if defined(__GNUC__) || defined(__clang__)
#  define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#elif defined(_MSC_VER)
#  define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#else
#  error "Packed attributes for struct is not implemented for this compiler. This may result in a program working incorrectly."
#endif

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

#define EVM_WORD_SIZE 8
#define EVM_STACK_CAPACITY 1024
#define EVM_PROGRAM_CAPACITY 1024
#define EVM_NATIVES_CAPACITY 1024
#define EVM_MEMORY_CAPACITY (640 * 1000)

#define EASM_BINDINGS_CAPACITY 1024
#define EASM_DEFERRED_OPERANDS_CAPACITY 1024
#define EASM_COMMENT_CHAR ';'
#define EASM_PP_CHAR '#'
#define EASM_MAX_INCLUDE_LEVEL 64

#define ARENA_CAPACITY (640 * 1000)

typedef struct {
	size_t count;
	const char *data;
} String_View;

// printf macros for String_View
#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int) sv.count, sv.data
// USAGE:
//   String_View name = ...;
//   printf("Name: %"SV_Fmt"\n", SV_Arg(name));
//   printf("Name: "SV_Fmt"\n", SV_Arg(name));

String_View sv_from_cstr(const char *cstr);
bool sv_eq(String_View a, String_View b);
uint64_t sv_to_u64(String_View sv);
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
static_assert(sizeof(Word) == EVM_WORD_SIZE, "The EVM's Word is expected to be 64 bits");

Word word_u64(uint64_t u64);
Word word_i64(int64_t i64);
Word word_f64(double f64);
Word word_ptr(void *ptr);

typedef enum {
	ERR_OK = 0,
	ERR_STACK_OVERFLOW,
	ERR_STACK_UNDERFLOW,
	ERR_ILLEGAL_INST,
	ERR_ILLEGAL_INST_ACCESS,
	ERR_ILLEGAL_MEMORY_ACCESS,
	ERR_ILLEGAL_OPERAND,
	ERR_DIV_BY_ZERO,
	ERR_NULL_NATIVE,
} Err;

const char *err_as_cstr(Err err);

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
	INST_MODI,
	INST_MULTU,
    	INST_DIVU,
    	INST_MODU,
	INST_PLUSF,
	INST_MINUSF,
	INST_MULTF,
	INST_DIVF,
	INST_JMP,
	INST_JMP_IF,
	INST_RET,
	INST_CALL,
	INST_NATIVE,
	INST_NOT,
	INST_EQI,
	INST_GEI,
	INST_GTI,
	INST_LEI,
	INST_LTI,
	INST_NEI,
	INST_EQF,
	INST_GEF,
	INST_GTF,
	INST_LEF,
	INST_LTF,
	INST_NEF,
	INST_EQU,
	INST_GEU,
	INST_GTU,
	INST_LEU,
	INST_LTU,
	INST_NEU,
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
	INST_I2F,
    	INST_U2F,
    	INST_F2I,
    	INST_F2U,
	INST_HALT,
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

typedef Err (*Evm_Native)(EVM *);

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

Err evm_execute_inst(EVM *evm);
Err evm_execute_program(EVM *evm, int limit);
void evm_push_native(EVM *evm, Evm_Native native);
void evm_dump_stack(FILE *stream, const EVM *evm);
void evm_dump_memory(FILE *stream, const EVM *evm);
void evm_push_inst(EVM *evm, Inst inst);
void evm_load_program_from_file(EVM *evm, const char *file_path);

#define EVM_FILE_MAGIC 0x6D65
#define EVM_FILE_VERSION 4

PACK(struct Evm_File_Meta {
	uint16_t magic;
	uint16_t version;
	uint64_t program_size;
	uint64_t entry;
	uint64_t memory_size;
	uint64_t memory_capacity;
});

typedef struct Evm_File_Meta Evm_File_Meta;

// NOTE: https://en.wikipedia.org/wiki/Region-based_memory_management
typedef struct {
	char buffer[ARENA_CAPACITY];
	size_t size;
} Arena;

void *arena_alloc(Arena *arena, size_t size);
String_View arena_slurp_file(Arena *arena, String_View file_path);
const char *arena_sv_to_cstr(Arena *arena, String_View sv);
String_View arena_sv_concat2(Arena *arena, const char *a, const char *b);
const char *arena_cstr_concat2(Arena *arena, const char *a, const char *b);

typedef struct {
    String_View file_path;
    int line_number;
} File_Location;

#define FL_Fmt SV_Fmt":%d"
#define FL_Arg(location) SV_Arg(location.file_path), location.line_number

typedef enum {
    	BINDING_CONST = 0,
    	BINDING_LABEL,
    	BINDING_NATIVE,
} Binding_Kind;

const char *binding_kind_as_cstr(Binding_Kind kind);

typedef struct {
	Binding_Kind kind;
	String_View name;
	Word value;
	File_Location location;
} Binding;

typedef struct {
	Inst_Addr addr;
	String_View label;
	File_Location location;
} Deferred_Operand;

typedef struct {
	Binding bindings[EASM_BINDINGS_CAPACITY];
	size_t bindings_size;

	Deferred_Operand deferred_operands[EASM_DEFERRED_OPERANDS_CAPACITY];
	size_t deferred_operands_size;

	Inst program[EVM_PROGRAM_CAPACITY];
    	uint64_t program_size;
	Inst_Addr entry;
	bool has_entry;
	String_View deferred_entry_binding_name;
	File_Location entry_location;

    	uint8_t memory[EVM_MEMORY_CAPACITY];
    	size_t memory_size;
    	size_t memory_capacity;

	Arena arena;

	size_t include_level;
} EASM;

bool easm_resolve_binding(const EASM *easm, String_View name, Binding *binding);
bool easm_bind_value(EASM *easm, String_View name, Word word, Binding_Kind kind, File_Location location, Binding *existing_binding);
void easm_push_deferred_operand(EASM *easm, Inst_Addr addr, String_View name, File_Location location);
bool easm_translate_literal(EASM *easm, String_View sv, Word *output);
void easm_save_to_file(EASM *easm, const char *output_file_path);
Word easm_push_string_to_memory(EASM *easm, String_View sv);
void easm_translate_source(EASM *easm, String_View input_file_path);

void evm_load_standard_natives(EVM *evm);
Err evm_alloc(EVM *evm);
Err evm_free(EVM *evm);
Err evm_print_u64(EVM *evm);
Err evm_print_i64(EVM *evm);
Err evm_print_f64(EVM *evm);
Err evm_print_ptr(EVM *evm);
Err evm_print_memory(EVM *evm);
Err evm_write(EVM *evm);

#endif // EVM_H_

#ifdef EVM_IMPLEMENTATION

Word word_u64(uint64_t u64) {
	return (Word) { .as_u64 = u64 };
}

Word word_i64(int64_t i64) {
	return (Word) { .as_i64 = i64 };
}

Word word_f64(double f64) {
	return (Word) { .as_f64 = f64 };
}

Word word_ptr(void *ptr) {
	return (Word) { .as_ptr = ptr };
}

const char *err_as_cstr(Err err) {
	switch (err) {
		case ERR_OK:				return "ERR_OK";
		case ERR_STACK_OVERFLOW:		return "ERR_STACK_OVERFLOW";
		case ERR_STACK_UNDERFLOW:		return "ERR_STACK_UNDERFLOW";
		case ERR_ILLEGAL_INST:			return "ERR_ILLEGAL_INST";
		case ERR_ILLEGAL_INST_ACCESS: 		return "ERR_ILLEGAL_INST_ACCESS";
		case ERR_ILLEGAL_MEMORY_ACCESS: 	return "ERR_ILLEGAL_MEMORY_ACCESS";
		case ERR_ILLEGAL_OPERAND:		return "ERR_ILLEGAL_OPERAND";
		case ERR_DIV_BY_ZERO:			return "ERR_DIV_BY_ZERO";
		case ERR_NULL_NATIVE:			return "ERR_NULL_NATIVE";
		default: UNREACHABLE("NOT EXISTING ERR");
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
		case INST_MODI:		return "modi";
		case INST_MULTU:   	return "multu";
		case INST_DIVU:    	return "divu";
    		case INST_MODU:    	return "modu";
		case INST_PLUSF:       	return "plusf";
		case INST_MINUSF:      	return "minusf";
		case INST_MULTF:       	return "multf";
		case INST_DIVF:        	return "divf";
		case INST_JMP:         	return "jmp";
		case INST_JMP_IF:      	return "jmp_if";
		case INST_RET:		return "ret";
		case INST_CALL:		return "call";
		case INST_NATIVE:	return "native";
		case INST_NOT:		return "not";
		case INST_EQU:     	return "equ";
    		case INST_GEU:     	return "geu";
    		case INST_GTU:     	return "gtu";
    		case INST_LEU:     	return "leu";
    		case INST_LTU:     	return "ltu";
    		case INST_NEU:     	return "neu";
		case INST_EQF:     	return "eqf";
    		case INST_GEF:     	return "gef";
    		case INST_GTF:     	return "gtf";
    		case INST_LEF:     	return "lef";
    		case INST_LTF:     	return "ltf";
    		case INST_NEF:     	return "nef";
    		case INST_EQI:     	return "eqi";
    		case INST_GEI:     	return "gei";
    		case INST_GTI:     	return "gti";
    		case INST_LEI:     	return "lei";
    		case INST_LTI:     	return "lti";
    		case INST_NEI:    	return "nei";
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
		case INST_I2F:     	return "i2f";
    		case INST_U2F:     	return "u2f";
    		case INST_F2I:     	return "f2i";
    		case INST_F2U:     	return "f2u";
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
		case INST_MODI:		return 0;
		case INST_MULTU:   	return 0;
    		case INST_DIVU:    	return 0;
    		case INST_MODU:    	return 0;
		case INST_PLUSF:       	return 0;
		case INST_MINUSF:      	return 0;
		case INST_MULTF:       	return 0;
		case INST_DIVF:        	return 0;
		case INST_JMP:         	return 1;
		case INST_JMP_IF:      	return 1;
		case INST_RET:		return 0;
		case INST_CALL:		return 1;
		case INST_NATIVE:	return 1;
		case INST_NOT:		return 0;
		case INST_EQU:     	return 0;
    		case INST_GEU:     	return 0;
    		case INST_GTU:     	return 0;
    		case INST_LEU:     	return 0;
    		case INST_LTU:     	return 0;
    		case INST_NEU:     	return 0;
		case INST_EQF:     	return 0;
    		case INST_GEF:     	return 0;
    		case INST_GTF:     	return 0;
    		case INST_LEF:     	return 0;
    		case INST_LTF:     	return 0;
    		case INST_NEF:     	return 0;
    		case INST_EQI:     	return 0;
    		case INST_GEI:     	return 0;
    		case INST_GTI:     	return 0;
    		case INST_LEI:     	return 0;
    		case INST_LTI:     	return 0;
    		case INST_NEI:    	return 0;
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
		case INST_I2F:     	return 0;
    		case INST_U2F:     	return 0;
    		case INST_F2I:     	return 0;
    		case INST_F2U:     	return 0;
		case EASM_NUMBER_OF_INSTS:
		default: UNREACHABLE("NOT EXISTING INST_TYPE");
	}
}

bool inst_by_name(String_View name, Inst_Type *type) {
	for (Inst_Type t = (Inst_Type) 0; t < EASM_NUMBER_OF_INSTS; ++t) {
		if (sv_eq(sv_from_cstr(inst_name(t)), name)) {
			*type = t;
			return true;
		}
	}
	return false;
}

#define BINARY_OP(evm, in, out, op)                                     \
    	do {                                                            \
        	if ((evm)->stack_size < 2) return ERR_STACK_UNDERFLOW;	\
        	(evm)->stack[(evm)->stack_size - 2].as_##out = (evm)->stack[(evm)->stack_size - 2].as_##in op (evm)->stack[(evm)->stack_size - 1].as_##in; \
        	(evm)->stack_size -= 1;                                	\
        	(evm)->ip += 1;                                         \
    	} while (false)

#define CAST_OP(evm, src, dst, cast)             			\
	do {                                        			\
        	if ((evm)->stack_size < 1) return ERR_STACK_UNDERFLOW;	\
        	(evm)->stack[(evm)->stack_size - 1].as_##dst = cast (evm)->stack[(evm)->stack_size - 1].as_##src; \
        	(evm)->ip += 1;                                         \
    	} while (false)

Err evm_execute_inst(EVM *evm) {
	if(evm->ip >= evm->program_size) return ERR_ILLEGAL_INST_ACCESS;

	Inst inst = evm->program[evm->ip];

	switch (inst.type) {
		case INST_NOP:
			evm->ip += 1;
		break;

		case INST_PUSH:
			if (evm->stack_size > EVM_STACK_CAPACITY) return ERR_STACK_OVERFLOW;
			evm->stack[evm->stack_size++] = inst.operand;
			evm->ip += 1;
		break;

		case INST_DROP:
			if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
			evm->stack_size -= 1;
			evm->ip += 1;
		break;

		case INST_DUP:
			if (evm->stack_size > EVM_STACK_CAPACITY) return ERR_STACK_OVERFLOW;
			if (evm->stack_size - inst.operand.as_u64 <= 0) return ERR_STACK_UNDERFLOW;
			evm->stack[evm->stack_size] = evm->stack[evm->stack_size - 1 - inst.operand.as_u64];
			evm->stack_size += 1;
			evm->ip += 1;
		break;

		case INST_PLUSI:
			BINARY_OP(evm, u64, u64, +);
		break;

		case INST_MINUSI:
			BINARY_OP(evm, u64, u64, -);
		break;

		case INST_MULTI:
		        BINARY_OP(evm, i64, i64, *);
        	break;

    		case INST_MULTU:
			BINARY_OP(evm, u64, u64, *);
		break;

		case INST_DIVI:
		        if (evm->stack[evm->stack_size - 1].as_i64 == 0) return ERR_DIV_BY_ZERO;
        		BINARY_OP(evm, i64, i64, /);
    		break;

    		case INST_DIVU:
			if (evm->stack[evm->stack_size - 1].as_u64 == 0) return ERR_DIV_BY_ZERO;
			BINARY_OP(evm, u64, u64, /);
		break;

		case INST_MODI:
		        if (evm->stack[evm->stack_size - 1].as_i64 == 0) return ERR_DIV_BY_ZERO;
        		BINARY_OP(evm, i64, i64, %);
    		break;

    		case INST_MODU:
			if (evm->stack[evm->stack_size - 1].as_u64 == 0) return ERR_DIV_BY_ZERO;
			BINARY_OP(evm, u64, u64, %);
		break;

		case INST_PLUSF:
			BINARY_OP(evm, f64, f64, +);
		break;

		case INST_MINUSF:
			BINARY_OP(evm, f64, f64, -);
		break;

		case INST_MULTF:
			BINARY_OP(evm, f64, f64, *);
		break;

		case INST_DIVF:
			BINARY_OP(evm, f64, f64, /);
		break;

		case INST_JMP:
			evm->ip = inst.operand.as_u64;
		break;

		case INST_JMP_IF:
			if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
			if (evm->stack[evm->stack_size - 1].as_u64) {
				evm->ip = inst.operand.as_u64;
			} else {
				evm->ip += 1;
			}
			evm->stack_size -= 1;
		break;

		case INST_RET:
			if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
			evm->ip = evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 1;
		break;

		case INST_CALL:
			if (evm->stack_size > EVM_STACK_CAPACITY) return ERR_STACK_OVERFLOW;
			evm->stack[evm->stack_size++].as_u64 = evm->ip + 1;
			evm->ip = inst.operand.as_u64;
		break;

		case INST_NATIVE:
			if (inst.operand.as_u64 > evm->natives_size) return ERR_ILLEGAL_OPERAND;
			if (!evm->natives[inst.operand.as_u64]) { return ERR_NULL_NATIVE; }
			const Err err = evm->natives[inst.operand.as_u64](evm);
			if (err != ERR_OK) return err;
			evm->ip += 1;
		break;

		case INST_NOT:
			if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
			evm->stack[evm->stack_size - 1].as_u64 = !evm->stack[evm->stack_size - 1].as_u64;
			evm->ip += 1;
		break;

		case INST_EQU:
        		BINARY_OP(evm, u64, u64, ==);
        	break;

    		case INST_GEU:
        		BINARY_OP(evm, u64, u64, >=);
        	break;

    		case INST_GTU:
        		BINARY_OP(evm, u64, u64, >);
        	break;

    		case INST_LEU:
        		BINARY_OP(evm, u64, u64, <=);
        	break;

    		case INST_LTU:
        		BINARY_OP(evm, u64, u64, <);
        	break;

    		case INST_NEU:
        		BINARY_OP(evm, u64, u64, !=);
        	break;

		case INST_EQF:
			BINARY_OP(evm, f64, u64, ==);
		break;

		case INST_GEF:
			BINARY_OP(evm, f64, u64, >=);
		break;

		case INST_GTF:
        		BINARY_OP(evm, f64, u64, >);
        	break;

    		case INST_LEF:
        		BINARY_OP(evm, f64, u64, <=);
        	break;

    		case INST_LTF:
        		BINARY_OP(evm, f64, u64, <);
        	break;

    		case INST_NEF:
        		BINARY_OP(evm, f64, u64, !=);
        	break;

    		case INST_EQI:
        		BINARY_OP(evm, i64, u64, ==);
        	break;

		case INST_GEI:
		        BINARY_OP(evm, i64, u64, >=);
        	break;

		case INST_GTI:
        		BINARY_OP(evm, i64, u64, >);
        	break;

    		case INST_LEI:
        		BINARY_OP(evm, i64, u64, <=);
        	break;

    		case INST_LTI:
        		BINARY_OP(evm, i64, u64, <);
        	break;

    		case INST_NEI:
        		BINARY_OP(evm, i64, u64, !=);
		break;

		case INST_HALT:
			evm->halt = true;
		break;

		case INST_SWAP:
			if (inst.operand.as_u64 >= evm->stack_size) return ERR_STACK_UNDERFLOW;
			const uint64_t a = evm->stack_size - 1;
			const uint64_t b = evm->stack_size - 1 - inst.operand.as_u64;
			Word t = evm->stack[a];
			evm->stack[a] = evm->stack[b];
			evm->stack[b] = t;
			evm->ip += 1;
		break;

		case INST_ANDB:
			BINARY_OP(evm, u64, u64, &);
        	break;

    		case INST_ORB:
			BINARY_OP(evm, u64, u64, |);
        	break;

    		case INST_XOR:
			BINARY_OP(evm, u64, u64, ^);
        	break;

    		case INST_SHR:
			BINARY_OP(evm, u64, u64, >>);
        	break;

    		case INST_SHL:
			BINARY_OP(evm, u64, u64, <<);
        	break;

    		case INST_NOTB:
        		if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
        		evm->stack[evm->stack_size - 1].as_u64 = ~evm->stack[evm->stack_size - 1].as_u64;
        		evm->ip += 1;
        	break;

		case INST_READ8: {
        		if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 1].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY) return ERR_ILLEGAL_MEMORY_ACCESS;
			evm->stack[evm->stack_size - 1].as_u64 = *(uint8_t*)&evm->memory[addr];
        		evm->ip += 1;
		} break;

		case INST_READ16: {
        		if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 1].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 1) return ERR_ILLEGAL_MEMORY_ACCESS;
			evm->stack[evm->stack_size - 1].as_u64 = *(uint16_t*)&evm->memory[addr];
        		evm->ip += 1;
		} break;

		case INST_READ32: {
        		if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 1].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 3) return ERR_ILLEGAL_MEMORY_ACCESS;
			evm->stack[evm->stack_size - 1].as_u64 = *(uint32_t*)&evm->memory[addr];
        		evm->ip += 1;
		} break;

		case INST_READ64: {
        		if (evm->stack_size < 1) return ERR_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 1].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 7) return ERR_ILLEGAL_MEMORY_ACCESS;
			evm->stack[evm->stack_size - 1].as_u64 = *(uint64_t*)&evm->memory[addr];
        		evm->ip += 1;
		} break;

		case INST_WRITE8: {
        		if (evm->stack_size < 2) return ERR_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY) return ERR_ILLEGAL_MEMORY_ACCESS;
			*(uint8_t*)&evm->memory[addr] = (uint8_t)evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 2;
        		evm->ip += 1;
		} break;

		case INST_WRITE16: {
        		if (evm->stack_size < 2) return ERR_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 1) return ERR_ILLEGAL_MEMORY_ACCESS;
			*(uint16_t*)&evm->memory[addr] = (uint16_t)evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 2;
        		evm->ip += 1;
		} break;

		case INST_WRITE32: {
        		if (evm->stack_size < 2) return ERR_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 3) return ERR_ILLEGAL_MEMORY_ACCESS;
			*(uint32_t*)&evm->memory[addr] = (uint32_t)evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 2;
        		evm->ip += 1;
		} break;

		case INST_WRITE64: {
        		if (evm->stack_size < 2) return ERR_STACK_UNDERFLOW;
			const Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
			if (addr >= EVM_MEMORY_CAPACITY - 7) return ERR_ILLEGAL_MEMORY_ACCESS;
			*(uint64_t*)&evm->memory[addr] = (uint64_t)evm->stack[evm->stack_size - 1].as_u64;
			evm->stack_size -= 2;
        		evm->ip += 1;
		} break;

		case INST_I2F:
        		CAST_OP(evm, i64, f64, (double));
        	break;

    		case INST_U2F:
        		CAST_OP(evm, u64, f64, (double));
        	break;

    		case INST_F2I:
        		CAST_OP(evm, f64, i64, (int64_t));
        	break;

    		case INST_F2U:
        		CAST_OP(evm, f64, u64, (uint64_t) (int64_t));
        	break;

    		case EASM_NUMBER_OF_INSTS:
		default:
			return ERR_ILLEGAL_INST;
	}

	return ERR_OK;
}

Err evm_execute_program(EVM *evm, int limit) {
	while (limit != 0 && !evm->halt) {
		Err err = evm_execute_inst(evm);
		if (err != ERR_OK) {
			return err;
		}

		if (limit > 0) --limit;
	}

	return ERR_OK;
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

void evm_load_program_from_file(EVM *evm, const char *file_path) {
	FILE *f = fopen(file_path, "rb");
	if (f == NULL) {
		fprintf(stderr, "ERROR: Could not open file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	Evm_File_Meta meta = { 0 };
	size_t n = fread(&meta, sizeof(meta), 1, f);
	if (n < 1) {
		fprintf(stderr, "ERROR: Could not read meta data from file %s: %s\n", file_path, strerror(errno));
		exit(1);
	}

	if (meta.magic != EVM_FILE_MAGIC) {
        	fprintf(stderr, "ERROR: %s does not appear to be a valid EVM file. Unexpected magic %04X. Expected %04X.\n", file_path, meta.magic, EVM_FILE_MAGIC);
		exit(1);
	}

	if (meta.version != EVM_FILE_VERSION) {
        	fprintf(stderr, "ERROR: %s: unsupported version of EVM file %d. Expected version %d.\n", file_path, meta.version, EVM_FILE_VERSION);
        	exit(1);
    	}

	if (meta.program_size > EVM_PROGRAM_CAPACITY) {
        	fprintf(stderr, "ERROR: %s: program section is too big. The file contains %lu program instruction. But the capacity is %lu\n", file_path, meta.program_size, (uint64_t) EVM_PROGRAM_CAPACITY);
		exit(1);
	}

	evm->ip = meta.entry;

	if (meta.memory_capacity > EVM_MEMORY_CAPACITY) {
        	fprintf(stderr, "ERROR: %s: memory section is too big. The file wants %lu bytes. But the capacity is %lu bytes\n", file_path, meta.memory_capacity, (uint64_t) EVM_MEMORY_CAPACITY);
        	exit(1);
    	}

	if (meta.memory_size > meta.memory_capacity) {
        fprintf(stderr,
                "ERROR: %s: memory size %lu is greater than declared memory capacity %lu\n", file_path, meta.memory_size, meta.memory_capacity);
        	exit(1);
    	}

    	evm->program_size = fread(evm->program, sizeof(evm->program[0]), meta.program_size, f);

    	if (evm->program_size != meta.program_size) {
        	fprintf(stderr, "ERROR: %s: read %zd program instructions, but expected %lu\n", file_path, evm->program_size, meta.program_size);
        	exit(1);
    	}

    	n = fread(evm->memory, sizeof(evm->memory[0]), meta.memory_size, f);

    	if (n != meta.memory_size) {
        	fprintf(stderr, "ERROR: %s: read %zd bytes of memory section, but expected %lu bytes.\n", file_path, n, meta.memory_size);
		exit(1);
	}

	fclose(f);
}

String_View sv_from_cstr(const char *cstr) {
	return (String_View) {
		.count = strlen(cstr),
		.data = cstr,
	};
}

bool sv_eq(String_View a, String_View b) {
	if (a.count != b.count) return false;
	else return (memcmp(a.data, b.data, a.count) == 0);
}

uint64_t sv_to_u64(String_View sv) {
	uint64_t result = 0;

	for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
        	result = result * 10 + (uint64_t)sv.data[i] - '0';
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

void *arena_alloc(Arena *arena, size_t size) {
	assert(arena->size + size <= ARENA_CAPACITY);

	void *result = arena->buffer + arena->size;
	arena->size += size;
	return result;
}

bool easm_resolve_binding(const EASM *easm, String_View name, Binding *binding) {
	for (size_t i = 0; i < easm->bindings_size; ++i) {
		if (sv_eq(easm->bindings[i].name, name)) {
			if (binding) *binding = easm->bindings[i];
			return true;
		}
	}

	return false;
}

bool easm_bind_value(EASM *easm, String_View name, Word word, Binding_Kind kind, File_Location location, Binding *existing_binding) {
	assert(easm->bindings_size < EASM_BINDINGS_CAPACITY);
	if (easm_resolve_binding(easm, name, existing_binding)) return false;
	easm->bindings[easm->bindings_size++] = (Binding) {
		.name = name,
		.value = word,
		.kind = kind,
		.location = location,
	};
	return true;
}

void easm_push_deferred_operand(EASM *easm, Inst_Addr addr, String_View label, File_Location location) {
	assert(easm->deferred_operands_size < EASM_DEFERRED_OPERANDS_CAPACITY);
	easm->deferred_operands[easm->deferred_operands_size++] = (Deferred_Operand) {
		.addr = addr,
		.label = label,
		.location = location,
	};
}

void easm_save_to_file(EASM *easm, const char *file_path) {
	FILE *f = fopen(file_path, "wb");
    	if (f == NULL) {
        	fprintf(stderr, "ERROR: Could not open file `%s`: %s\n", file_path, strerror(errno));
        	exit(1);
    	}

	Evm_File_Meta meta = {
		.magic = EVM_FILE_MAGIC,
		.version = EVM_FILE_VERSION,
		.program_size = easm->program_size,
		.entry = easm->entry,
		.memory_size = easm->memory_size,
		.memory_capacity = easm->memory_capacity,
	};

	fwrite(&meta, sizeof(meta), 1, f);
    	if (ferror(f)) {
        	fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n", file_path, strerror(errno));
        	exit(1);
    	}

    	fwrite(easm->program, sizeof(easm->program[0]), easm->program_size, f);
    	if (ferror(f)) {
        	fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n", file_path, strerror(errno));
        	exit(1);
    	}

    	fwrite(easm->memory, sizeof(easm->memory[0]), easm->memory_size, f);
    	if (ferror(f)) {
        	fprintf(stderr, "ERROR: Could not write to file `%s`: %s\n", file_path, strerror(errno));
        	exit(1);
    	}

    	fclose(f);
}

void easm_translate_source(EASM *easm, String_View input_file_path) {
	String_View original_source = arena_slurp_file(&easm->arena, input_file_path);
	String_View source = original_source;
	File_Location location = {
        	.file_path = input_file_path,
    	};

	// First pass
	while (source.count > 0) {
		String_View line = sv_trim(sv_chop_by_delim(&source, '\n'));
		line = sv_trim(sv_chop_by_delim(&line, EASM_COMMENT_CHAR));
		location.line_number += 1;
		if (line.count > 0) {
			String_View token = sv_trim(sv_chop_by_delim(&line, ' '));

			// Pre-processor
			if (token.count > 0 && token.data[0] == EASM_PP_CHAR) {
				token.count -= 1;
				token.data += 1;
				if (sv_eq(token, sv_from_cstr("const"))) {
					line = sv_trim(line);
					String_View label = sv_chop_by_delim(&line, ' ');
					if (label.count > 0) {
						line = sv_trim(line);
						String_View value = line;
						Word word = { 0 };
						if (!easm_translate_literal(easm, value, &word)) {
							fprintf(stderr, FL_Fmt": ERROR: unknown pre-processor directive '"SV_Fmt"'\n", FL_Arg(location), SV_Arg(value));
							exit(1);
						}

						Binding existing = {0};
                        			if (!easm_bind_value(easm, label, word, BINDING_CONST, location, &existing)) {
							fprintf(stderr, FL_Fmt": ERROR: label '"SV_Fmt"' is allready define\n", FL_Arg(location), SV_Arg(label));
	                    				fprintf(stderr, FL_Fmt": NOTE: first binding is located here\n", FL_Arg(existing.location));
							exit(1);
						}
					} else {
						fprintf(stderr, FL_Fmt": ERROR: label name in not provided\n", FL_Arg(location));
						exit(1);
					}
				} else if (sv_eq(token, sv_from_cstr("native"))) {
                    			line = sv_trim(line);
                    			String_View name = sv_chop_by_delim(&line, ' ');
                    			if (name.count > 0) {
                        			line = sv_trim(line);
                        			String_View value = line;
                        			Word word = {0};
                        			if (!easm_translate_literal(easm, value, &word)) {
                            				fprintf(stderr, FL_Fmt": ERROR: '"SV_Fmt"' is not a number", FL_Arg(location), SV_Arg(value));
                            				exit(1);
                        			}

						Binding existing = {0};
                        			if (!easm_bind_value(easm, name, word, BINDING_NATIVE, location, &existing)) {
                            				fprintf(stderr, FL_Fmt": ERROR: name `"SV_Fmt"` is already bound\n", FL_Arg(location), SV_Arg(name));
	                    				fprintf(stderr, FL_Fmt": NOTE: first binding is located here\n", FL_Arg(existing.location));
                            				exit(1);
                        			}
                    			} else {
                        			fprintf(stderr, FL_Fmt": ERROR: binding name is not provided\n", FL_Arg(location));
                        			exit(1);
                    			}
				} else if (sv_eq(token, sv_from_cstr("include"))) {
					line = sv_trim(line);
					if (line.count > 0) {
						if (line.data[0] == '"' && line.data[line.count - 1] == '"') {
							line.count -= 2;
							line.data += 1;

							if (easm->include_level + 1 >= EASM_MAX_INCLUDE_LEVEL) {
								fprintf(stderr, FL_Fmt": ERROR: exceeded maximum include level\n", FL_Arg(location));
								exit(1);
							}

							easm->include_level += 1;
							easm_translate_source(easm, line);
							easm->include_level -= 1;
						} else {
							fprintf(stderr, FL_Fmt": ERROR: path must be surrounded by quotation marks\n", FL_Arg(location));
							exit(1);
						}
					} else {
						fprintf(stderr, FL_Fmt": ERROR: include file path is not provided\n", FL_Arg(location));
						exit(1);
					}
				} else if (sv_eq(token, sv_from_cstr("entry"))) {
					if (easm->has_entry) {
						fprintf(stderr, FL_Fmt": ERROR: entry point has been already set!\n", FL_Arg(location));
						fprintf(stderr, FL_Fmt": NOTE: the first entry point\n", FL_Arg(easm->entry_location));
						exit(1);
					}

					line = sv_trim(line);
					if (line.count == 0) {
						fprintf(stderr, FL_Fmt": ERROR: entry point is not specified\n", FL_Arg(location));
						exit(1);
					}

					Word entry = { 0 };

					if (!easm_translate_literal(easm, line, &entry)) {
						easm->deferred_entry_binding_name = line;
					} else {
						easm->entry = entry.as_u64;
					}

					easm->has_entry = true;
					easm->entry_location = location;
			 	} else {
					fprintf(stderr, FL_Fmt": ERROR: unknown pre-processor directive '"SV_Fmt"'\n", FL_Arg(location), SV_Arg(token));
					exit(1);
				}
			} else {
				// Binding
				if (token.count > 0 && token.data[token.count - 1] == ':') {
					String_View label = {
						.count = token.count - 1,
						.data = token.data,
					};

					Binding existing = {0};
					if (!easm_bind_value(easm, label, word_u64(easm->program_size), BINDING_LABEL, location, &existing)) {
						fprintf(stderr, FL_Fmt": ERROR: label '"SV_Fmt"' is already define\n", FL_Arg(location), SV_Arg(label));
						fprintf(stderr, FL_Fmt": NOTE: the first entry point\n", FL_Arg(easm->entry_location));
						exit(1);
					}
					token = sv_trim(sv_chop_by_delim(&line, ' '));
				}

				// Instraction
				if (token.count > 0) {
					String_View operand = line;

					Inst_Type inst_type = INST_NOP;
					if (inst_by_name(token, &inst_type)) {
						assert(easm->program_size < EVM_PROGRAM_CAPACITY);
						easm->program[easm->program_size].type = inst_type;
						if (inst_has_operand(inst_type)) {
							if (operand.count == 0) {
								fprintf(stderr, FL_Fmt": ERROR: instruction '"SV_Fmt"' requires an operand\n", FL_Arg(location), SV_Arg(token));
								exit(1);
							}
							if (!easm_translate_literal(easm, operand, &easm->program[easm->program_size].operand)) {
								easm_push_deferred_operand(easm, easm->program_size, operand, location);
							}
						}
						easm->program_size += 1;
					} else {
						fprintf(stderr, FL_Fmt": ERROR: unknown instruction '"SV_Fmt"'\n", FL_Arg(location), SV_Arg(token));
						exit(1);
					}
				}
			}
		}
	}

	// Second pass
	for (size_t i = 0; i < easm->deferred_operands_size; ++i) {
		String_View label = easm->deferred_operands[i].label;
		Inst_Addr addr = easm->deferred_operands[i].addr;
		Binding binding = {0};
		if (!easm_resolve_binding(easm, label, &binding)) {
			fprintf(stderr, FL_Fmt": ERROR: unknown label '"SV_Fmt"'\n", FL_Arg(easm->deferred_operands[i].location), SV_Arg(label));
			exit(1);
		}

		if (easm->program[addr].type == INST_CALL && binding.kind != BINDING_LABEL) {
            		fprintf(stderr, FL_Fmt": ERROR: trying to call not a label. `"SV_Fmt"` is %s, but the call instructions accepts only literals or bindings.\n", FL_Arg(easm->deferred_operands[i].location), SV_Arg(label), binding_kind_as_cstr(binding.kind));
            		exit(1);
        	}

        	if (easm->program[addr].type == INST_NATIVE && binding.kind != BINDING_NATIVE) {
            		fprintf(stderr, FL_Fmt": ERROR: trying to invoke native function from a binding that is %s. Bindings for native functions have to be defined via `%%native` easm directive.\n", FL_Arg(easm->deferred_operands[i].location), binding_kind_as_cstr(binding.kind));
            		exit(1);
       	 	}

		easm->program[addr].operand = binding.value;
	}

	// Resolving deferred entry point
	if (easm->has_entry && easm->deferred_entry_binding_name.count > 0) {
		Binding binding = {0};
		if (!easm_resolve_binding(easm, easm->deferred_entry_binding_name, &binding)) {
			fprintf(stderr, FL_Fmt": ERROR: unknown label '"SV_Fmt"'\n", FL_Arg(easm->entry_location), SV_Arg(easm->deferred_entry_binding_name));
			exit(1);
		}

		if (binding.kind != BINDING_LABEL) {
            		fprintf(stderr, FL_Fmt": ERROR: trying to set a %s as an entry point. Entry point has to be a label.\n", FL_Arg(easm->entry_location), binding_kind_as_cstr(binding.kind));
            		exit(1);
        	}

		easm->entry = binding.value.as_u64;
	}
}

Word easm_push_string_to_memory(EASM *easm, String_View sv) {
    assert(easm->memory_size + sv.count <= EVM_MEMORY_CAPACITY);

    Word result = word_u64(easm->memory_size);
    memcpy(easm->memory + easm->memory_size, sv.data, sv.count);
    easm->memory_size += sv.count;

    if (easm->memory_size > easm->memory_capacity) {
        easm->memory_capacity = easm->memory_size;
    }

    return result;
}

const char *binding_kind_as_cstr(Binding_Kind kind) {
    switch (kind) {
        case BINDING_CONST: return "const";
        case BINDING_LABEL: return "label";
        case BINDING_NATIVE: return "native";
        default:
            UNREACHABLE("binding_kind_as_cstr: unreachable");
            exit(0);
    }
}

bool easm_translate_literal(EASM *easm, String_View sv, Word *output) {
    	if (sv.count >= 2 && *sv.data == '\'' && sv.data[sv.count - 1] == '\'') {
        	if (sv.count - 2 != 1) {
            		return false;
        	}

        	*output = word_u64((uint64_t) sv.data[1]);

        	return true;
	} else if (sv.count >= 2 && *sv.data == '"' && sv.data[sv.count - 1] == '"') {
		// TODO: string literals don't support escaped characters
        	sv.data += 1;
        	sv.count -= 2;
        	*output = easm_push_string_to_memory(easm, sv);
	} else {
		const char *cstr = arena_sv_to_cstr(&easm->arena, sv);

		char *endptr = 0;
		Word result = { 0 };
		result.as_u64 = strtoull(cstr, &endptr, 10);
		if ((size_t) (endptr - cstr) != sv.count) {
			result.as_f64 = strtod(cstr, &endptr);
			if ((size_t) (endptr - cstr) != sv.count) return false;
		}

		*output = result;
	}
	return true;
}

String_View arena_slurp_file(Arena *arena, String_View file_path) {
	const char *file_path_cstr = arena_sv_to_cstr(arena, file_path);
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

	char *buffer = arena_alloc(arena, (size_t)m);
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

const char *arena_sv_to_cstr(Arena *arena, String_View sv) {
	char *cstr = arena_alloc(arena, sv.count + 1);
	memcpy(cstr, sv.data, sv.count);
	cstr[sv.count] = '\0';
	return cstr;
}

const char *arena_cstr_concat2(Arena *arena, const char *a, const char *b) {
	const size_t a_len = strlen(a);
	const size_t b_len = strlen(b);
	char *buf = arena_alloc(arena, a_len + b_len + 1);
	memcpy(buf, a, a_len);
	memcpy(buf + a_len, b, b_len);
	buf[a_len + b_len] = '\0';
	return buf;
}

String_View arena_sv_concat2(Arena *arena, const char *a, const char *b) {
    	const size_t a_len = strlen(a);
    	const size_t b_len = strlen(b);
    	char *buf = arena_alloc(arena, a_len + b_len);
    	memcpy(buf, a, a_len);
    	memcpy(buf + a_len, b, b_len);
    	return (String_View) {
        	.count = a_len + b_len,
        	.data = buf,
	};
}

void evm_load_standard_natives(EVM *evm) {
	evm_push_native(evm, evm_write);	// 0
}

Err evm_write(EVM *evm) {
	if (evm->stack_size < 2) return ERR_STACK_UNDERFLOW;
	Memory_Addr addr = evm->stack[evm->stack_size - 2].as_u64;
	uint64_t count = evm->stack[evm->stack_size - 1].as_u64;

	if (addr >= EVM_MEMORY_CAPACITY) return ERR_ILLEGAL_MEMORY_ACCESS;
	if (addr + count < addr || addr + count >= EVM_MEMORY_CAPACITY) return ERR_ILLEGAL_MEMORY_ACCESS;

	fwrite(&evm->memory[addr], sizeof(evm->memory[0]), count, stdout);
	evm->stack_size -= 2;

	return ERR_OK;
}

#endif //EVM_IMPLEMENTATION
