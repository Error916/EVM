#include <stdio.h>
#include <stdlib.h>

#define EVM_IMPLEMENTATION
#include "./evm.h"

static void usage(FILE *f) {
		fprintf(f, "Usage: easm2nasm <input.easm>\n");
}

int main(int argc, char **argv) {
	if (argc < 2) {
		usage(stderr);
		fprintf(stderr, "ERROR: no input provided\n");
		exit(1);
	}

	// NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
	static EASM easm = { 0 };
	easm_translate_source(&easm, sv_form_cstr(argv[1]));

	printf("BITS 64\n");
	printf("%%define EVM_STACK_CAPACITY %d\n", EVM_STACK_CAPACITY);
	printf("%%define EVM_WORD_SIZE %d\n", EVM_WORD_SIZE);
	printf("%%define STDOUT 1\n");
	printf("%%define SYS_EXIT 60\n");
	printf("%%define SYS_WRITE 1\n");
	printf("segment .text\n");
	printf("global _start\n");

	size_t jmp_if_escape_count = 0;
	for (size_t i = 0; i < easm.program_size; ++i) {
		Inst inst = easm.program[i];

		if (i == easm.entry) {
	    		printf("_start:\n");
		}

		printf("inst_%zu:\n", i);
		switch (inst.type) {
			case INST_NOP: UNIMPLEMENTED("INST_NOP");

			case INST_PUSH: {
				printf("\t;; push %lu\n", inst.operand.as_u64);
				printf("\tmov rsi, [stack_top]\n");
				printf("\tmov rax, 0x%lx\n", inst.operand.as_u64);
				printf("\tmov QWORD [rsi], rax\n");
				printf("\tadd QWORD [stack_top], EVM_WORD_SIZE\n");
			} break;

			case INST_DROP: {
				printf("\t;; drop\n");
				printf("\tmov rsi, [stack_top]\n");
	    			printf("\tsub rsi, EVM_WORD_SIZE\n");
	    			printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_DUP: {
				printf("\t;; dup %lu\n", inst.operand.as_u64);
				printf("\tmov rsi, [stack_top]\n");
				printf("\tmov rdi, rsi\n");
				printf("\tsub rdi, EVM_WORD_SIZE * (%lu + 1)\n", inst.operand.as_u64);
				printf("\tmov rax, [rdi]\n");
				printf("\tmov [rsi], rax\n");
				printf("\tadd rsi, EVM_WORD_SIZE\n");
				printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_SWAP: {
				printf("\t;; swap %lu\n", inst.operand.as_u64);
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rdi, rsi\n");
				printf("\tsub rdi, EVM_WORD_SIZE * %lu\n", inst.operand.as_u64);
				printf("\tmov rax, [rsi]\n");
				printf("\tmov rbx, [rdi]\n");
				printf("\tmov [rdi], rax\n");
				printf("\tmov [rsi], rbx\n");
			} break;

			case INST_PLUSI: {
				printf("\t;; plusi \n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rbx, [rsi]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\tadd rax, rbx\n");
				printf("\tmov [rsi], rax\n");
				printf("\tadd rsi, EVM_WORD_SIZE\n");
				printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_MINUSI: {
				printf("\t;; minusi \n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rbx, [rsi]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\tsub rax, rbx\n");
				printf("\tmov [rsi], rax\n");
				printf("\tadd rsi, EVM_WORD_SIZE\n");
				printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_MULTI: {
	    			printf("\t;; FIXME: multi\n");
			} break;

			case INST_MULTU: UNIMPLEMENTED("INST_MULTU");

			case INST_DIVI: {
	    			printf("\t;; divi\n");
	    			printf("\tmov rsi, [stack_top]\n");
	    			printf("\tsub rsi, EVM_WORD_SIZE\n");
	    			printf("\tmov rbx, [rsi]\n");
	    			printf("\tsub rsi, EVM_WORD_SIZE\n");
	    			printf("\tmov rax, [rsi]\n");
	    			printf("\txor rdx, rdx\n");
	    			printf("\tidiv rbx\n");
	    			printf("\tmov [rsi], rax\n");
	    			printf("\tadd rsi, EVM_WORD_SIZE\n");
	    			printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_DIVU: {
				printf("\t;; divi\n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rbx, [rsi]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\txor rdx, rdx\n");
				printf("\tdiv rbx\n");
				printf("\tmov [rsi], rax\n");
				printf("\tadd rsi, EVM_WORD_SIZE\n");
				printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_MODI: {
				printf("\t;; modi\n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rbx, [rsi]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\txor rdx, rdx\n");
				printf("\tidiv rbx\n");
				printf("\tmov [rsi], rdx\n");
				printf("\tadd rsi, EVM_WORD_SIZE\n");
				printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_MODU: {
				printf("\t;; modi\n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rbx, [rsi]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\txor rdx, rdx\n");
				printf("\tdiv rbx\n");
				printf("\tmov [rsi], rdx\n");
				printf("\tadd rsi, EVM_WORD_SIZE\n");
				printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_PLUSF: {
	    			printf("\t;; FIXME: plusf\n");
			} break;

			case INST_MINUSF: {
	    			printf("\t;; FIXME: minusf\n");
			} break;

			case INST_MULTF: {
	    			printf("\t;; FIXME: multf\n");
			} break;

			case INST_DIVF: {
	    			printf("\t;; FIXME: divf\n");
			} break;

			case INST_JMP: {
				printf("\t;; jmp\n");
				printf("\tmov rdi, inst_map\n");
				printf("\tadd rdi, EVM_WORD_SIZE * %lu\n", inst.operand.as_u64);
				printf("\tjmp [rdi]\n");
			} break;

			case INST_JMP_IF: {
				printf("\t;; jmp_if %lu\n", inst.operand.as_u64);
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\tmov [stack_top], rsi\n");
				printf("\tcmp rax, 0\n");
				printf("\tje jmp_if_escape_%zu\n", jmp_if_escape_count);
				printf("\tmov rdi, inst_map\n");
				printf("\tadd rdi, EVM_WORD_SIZE * %lu\n", inst.operand.as_u64);
				printf("\tjmp [rdi]\n");
				printf("jmp_if_escape_%zu:\n", jmp_if_escape_count);
				jmp_if_escape_count += 1;
			} break;

			case INST_RET: {
				printf("\t;; ret\n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\tmov rbx, EVM_WORD_SIZE\n");
				printf("\tmul rbx\n");
				printf("\tadd rax, inst_map\n");
				printf("\tmov [stack_top], rsi\n");
				printf("\tjmp [rax]\n");
			} break;

			case INST_CALL: {
				printf("\t;; call\n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tmov QWORD [rsi], %zu\n", i + 1);
				printf("\tadd rsi, EVM_WORD_SIZE\n");
				printf("\tmov [stack_top], rsi\n");
				printf("\tmov rdi, inst_map\n");
				printf("\tadd rdi, EVM_WORD_SIZE * %lu\n", inst.operand.as_u64);
				printf("\tjmp [rdi]\n");
			} break;

			case INST_NATIVE: {
				if (inst.operand.as_u64 == 3) {
					printf("\t;; native print_i64\n");
					printf("\tcall print_i64\n");
				} else if (inst.operand.as_u64 == 7) {
					printf("\t;; native write\n");
					printf("\tmov r11, [stack_top]\n");
					printf("\tsub r11, EVM_WORD_SIZE\n");
					printf("\tmov rdx, [r11]\n");
					printf("\tsub r11, EVM_WORD_SIZE\n");
					printf("\tmov rsi, [r11]\n");
					printf("\tadd rsi, memory\n");
					printf("\tmov rdi, STDOUT\n");
					printf("\tmov rax, SYS_WRITE\n");
					printf("\tmov [stack_top], r11\n");
					printf("\tsyscall\n");
				} else UNIMPLEMENTED("Unsupported native function");
			} break;

			case INST_NOT: {
				printf("\t;; not\n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\tcmp rax, 0\n");
				printf("\tmov rax, 0\n");
				printf("\tsetz al\n");
				printf("\tmov [rsi], rax\n");
	    		} break;

			case INST_EQI: {
				printf("\t;; eqi\n");
				printf("\tmov rsi, [stack_top]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rbx, [rsi]\n");
				printf("\tsub rsi, EVM_WORD_SIZE\n");
				printf("\tmov rax, [rsi]\n");
				printf("\tcmp rax, rbx\n");
				printf("\tmov rax, 0\n");
				printf("\tsetz al\n");
				printf("\tmov [rsi], rax\n");
				printf("\tadd rsi, EVM_WORD_SIZE\n");
				printf("\tmov [stack_top], rsi\n");
			} break;

			case INST_GEI: {
	    			printf("\t;; FIXME: gei\n");
			} break;

			case INST_GTI: {
	    			printf("\t;; FIXME: gti\n");
			} break;

			case INST_LEI: {
	    			printf("\t;; FIXME: lei\n");
			} break;

			case INST_LTI: UNIMPLEMENTED("INST_LTI");
			case INST_NEI: UNIMPLEMENTED("INST_NEI");

			case INST_EQU: {
	    			printf("\t;; FIXME: equ\n");
			} break;

			case INST_GEU: UNIMPLEMENTED("INST_GEU");
			case INST_GTU: UNIMPLEMENTED("INST_GTU");
			case INST_LEU: UNIMPLEMENTED("INST_LEU");
			case INST_LTU: UNIMPLEMENTED("INST_LTU");
			case INST_NEU: UNIMPLEMENTED("INST_NEU");
			case INST_EQF: UNIMPLEMENTED("INST_EQF");

			case INST_GEF: {
			    	printf("\t;; FIXME: gef\n");
			} break;

			case INST_GTF: {
			    	printf("\t;; FIXME: gtf\n");
			} break;

			case INST_LEF: {
			    	printf("\t;; FIXME: lef\n");
			} break;

			case INST_LTF: {
			    	printf("\t;; FIXME: ltf\n");
			} break;

			case INST_NEF: UNIMPLEMENTED("INST_NEF");

			case INST_ANDB: {
	    			printf("\t;; FIXME: andb\n");
			} break;

			case INST_ORB: UNIMPLEMENTED("INST_ORB");

			case INST_XOR: {
	    			printf("\t;; FIXME: xor\n");
			} break;

			case INST_SHR: UNIMPLEMENTED("INST_SHR");
			case INST_SHL: UNIMPLEMENTED("INST_SHL");
			case INST_NOTB: UNIMPLEMENTED("INST_NOTB");

			case INST_READ8: {
				printf("\t;; read8\n");
				printf("\tmov r11, [stack_top]\n");
				printf("\tsub r11, EVM_WORD_SIZE\n");
				printf("\tmov rsi, [r11]\n");
				printf("\tadd rsi, memory\n");
				printf("\txor rax, rax\n");
				printf("\tmov al, BYTE [rsi]\n");
				printf("\tmov [r11], rax\n");
			} break;

			case INST_READ16: UNIMPLEMENTED("INST_READ16");
			case INST_READ32: UNIMPLEMENTED("INST_READ32");
			case INST_READ64: UNIMPLEMENTED("INST_READ64");

			case INST_WRITE8: {
				printf("\t;; write8\n");
				printf("\tmov r11, [stack_top]\n");
				printf("\tsub r11, EVM_WORD_SIZE\n");
				printf("\tmov rax, [r11]\n");
				printf("\tsub r11, EVM_WORD_SIZE\n");
				printf("\tmov rsi, [r11]\n");
				printf("\tadd rsi, memory\n");
				printf("\tmov BYTE [rsi], al\n");
				printf("\tmov [stack_top], r11\n");
			} break;

			case INST_WRITE16: UNIMPLEMENTED("INST_WRITE16");
			case INST_WRITE32: UNIMPLEMENTED("INST_WRITE32");
			case INST_WRITE64: UNIMPLEMENTED("INST_WRITE64");

			case INST_I2F: {
	    			printf("\t;; FIXME: i2f\n");
			} break;

			case INST_U2F: UNIMPLEMENTED("INST_U2F");

			case INST_F2I: {
			    	printf("\t;; FIXME: f2i\n");
			} break;

			case INST_F2U: UNIMPLEMENTED("INST_F2U");

			case INST_HALT: {
				printf("\t;; halt\n");
				printf("\tmov rax, SYS_EXIT\n");
				printf("\tmov rdi, 0\n");
				printf("\tsyscall\n");
			} break;

			case EASM_NUMBER_OF_INSTS:
			default: UNREACHABLE("NOT EXISTING INST_TYPE");
		}
	}

	printf("\tret\n");
	printf("segment .data\n");
	printf("stack_top: dq stack\n");
	printf("inst_map: dq");
    	for (size_t i = 0; i < easm.program_size; ++i) {
		printf(" inst_%zu,", i);
    	}
	printf("\n");
    	printf("memory:\n");
#define ROW_SIZE 10
#define ROW_COUNT(size) ((size + ROW_SIZE - 1) / ROW_SIZE)
    	for (size_t row = 0; row < ROW_COUNT(easm.memory_size); ++row) {
		printf("\tdb");
		for (size_t col = 0; col < ROW_SIZE && row * ROW_SIZE + col < easm.memory_size; ++col) {
			printf(" %u,", easm.memory[row * ROW_SIZE + col]);
		}
		printf("\n");
    	}
    	// TODO: warning: uninitialized space declared in non-BSS section `.data': zeroing
    	//   Is it possible to get rid of it? Not really suppress it, but just let nasm know
    	//   that I know what I'm doing.
    	printf("\ttimes %zu db 0", EVM_MEMORY_CAPACITY - easm.memory_size);
#undef ROW_SIZE
#undef ROW_COUNT
    	printf("\n");
	printf("segment .bss\n");
	printf("stack: resq EVM_STACK_CAPACITY\n");

	return 0;
}
