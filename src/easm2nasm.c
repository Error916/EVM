#include <stdio.h>
#include <stdlib.h>

#define EVM_IMPLEMENTATION
#include "./evm.h"

static void gen_print_i64(void) {
	printf("print_i64:\n");
	printf("\t;; extract value from EVM stack\n");
	printf("\tmov rsi, [stack_top]\n");
	printf("\tsub rsi, EVM_WORD_SIZE\n");
	printf("\tmov rax, [rsi]\n");
	printf("\tmov [stack_top], rsi\n");
	printf("\t;; rax contains the value we need to print\n");
	printf("\t;; rdi is the char counter\n");
	printf("\tmov rdi, 0\n");
	printf("\t;; add the new line\n");
	printf("\tdec rsp\n");
	printf("\tinc rdi\n");
	printf("\tmov BYTE [rsp], 10\n");
	printf(".loop:\n");
	printf("\txor rdx, rdx\n");
	printf("\tmov rbx, 10\n");
	printf("\tdiv rbx\n");
	printf("\tadd rdx, '0'\n");
	printf("\tdec rsp\n");
	printf("\tinc rdi\n");
	printf("\tmov [rsp], dl\n");
	printf("\tcmp rax, 0\n");
	printf("\tjne .loop\n");
	printf("\t;; rsp - points at the begining of the buf\n");
	printf("\t;; rdi - cointain the size of the buf\n");
	printf("\t;; printing the buffer\n");
	printf("\tmov rbx, rdi\n");
	printf("\t;; write(STDOUT, buf, buf_size)\n");
	printf("\tmov rax, SYS_WRITE\n");
	printf("\tmov rdi, STDOUT\n");
	printf("\tmov rsi, rsp\n");
	printf("\tmov rdx, rbx\n");
	printf("\tsyscall\n");
	printf("\tadd rsp, rbx\n");
	printf("\tret\n");
}

static void usage(FILE *f) {
		fprintf(f, "Usage: easm2nasm <input.easm>\n");
}

EASM easm = { 0 };

int main(int argc, char **argv) {
	if (argc < 2) {
		usage(stderr);
		fprintf(stderr, "ERROR: no input provided\n");
		exit(1);
	}

	easm_translate_source(&easm, cstr_as_sv(argv[1]));

	printf("BITS 64\n");
	printf("%%define EVM_STACK_CAPACITY %d\n", EVM_STACK_CAPACITY);
	printf("%%define EVM_WORD_SIZE %d\n", EVM_WORD_SIZE);
	printf("%%define STDOUT 1\n");
	printf("%%define SYS_EXIT 60\n");
	printf("%%define SYS_WRITE 1\n");
	printf("segment .text\n");
	printf("global _start\n");
	gen_print_i64();
	printf("_start:\n");

	size_t jmp_if_escape_count = 0;
	for (size_t i = 0; i < easm.program_size; ++i) {
		Inst inst = easm.program[i];

		printf("inst_%zu:\n", i);
		switch (inst.type) {
			case INST_NOP: UNIMPLEMENTED("INST_NOP");

			case INST_PUSH: {
				printf("\t;; push %lu\n", inst.operand.as_u64);
				printf("\tmov rsi, [stack_top]\n");
				printf("\tmov QWORD [rsi], %lu\n", inst.operand.as_u64);
				printf("\tadd QWORD [stack_top], EVM_WORD_SIZE\n");
			} break;

			case INST_DROP: UNIMPLEMENTED("INST_DROP");

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

			case INST_MULTI: UNIMPLEMENTED("INST_MULTI");
			case INST_DIVI: UNIMPLEMENTED("INST_DIVI");
			case INST_MODI: UNIMPLEMENTED("INST_MODI");
			case INST_PLUSF: UNIMPLEMENTED("INST_PLUSF");
			case INST_MINUSF: UNIMPLEMENTED("INST_MINUSF");
			case INST_MULTF: UNIMPLEMENTED("INST_MULTF");
			case INST_DIVF: UNIMPLEMENTED("INST_DIVF");
			case INST_JMP: UNIMPLEMENTED("INST_JMP");

			case INST_JMP_IF: {
				printf("\t;; TODO: jmp_if %lu\n", inst.operand.as_u64);
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

			case INST_RET: UNIMPLEMENTED("INST_RET");
			case INST_CALL: UNIMPLEMENTED("INST_CALL");

			case INST_NATIVE: {
				if (inst.operand.as_u64 == 3) {
					printf("\t;; native print_i64\n");
					printf("\tcall print_i64\n");
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

			case INST_GEI: UNIMPLEMENTED("INST_GEI");
			case INST_GTI: UNIMPLEMENTED("INST_GTI");
			case INST_LEI: UNIMPLEMENTED("INST_LEI");
			case INST_LTI: UNIMPLEMENTED("INST_LTI");
			case INST_NEI: UNIMPLEMENTED("INST_NEI");
			case INST_EQF: UNIMPLEMENTED("INST_EQF");
			case INST_GEF: UNIMPLEMENTED("INST_GEF");
			case INST_GTF: UNIMPLEMENTED("INST_GTF");
			case INST_LEF: UNIMPLEMENTED("INST_LEF");
			case INST_LTF: UNIMPLEMENTED("INST_LTF");
			case INST_NEF: UNIMPLEMENTED("INST_NEF");
			case INST_ANDB: UNIMPLEMENTED("INST_ANDB");
			case INST_ORB: UNIMPLEMENTED("INST_ORB");
			case INST_XOR: UNIMPLEMENTED("INST_XOR");
			case INST_SHR: UNIMPLEMENTED("INST_SHR");
			case INST_SHL: UNIMPLEMENTED("INST_SHL");
			case INST_NOTB: UNIMPLEMENTED("INST_NOTB");
			case INST_READ8: UNIMPLEMENTED("INST_READ8");
			case INST_READ16: UNIMPLEMENTED("INST_READ16");
			case INST_READ32: UNIMPLEMENTED("INST_READ32");
			case INST_READ64: UNIMPLEMENTED("INST_READ64");
			case INST_WRITE8: UNIMPLEMENTED("INST_WRITE8");
			case INST_WRITE16: UNIMPLEMENTED("INST_WRITE16");
			case INST_WRITE32: UNIMPLEMENTED("INST_WRITE32");
			case INST_WRITE64: UNIMPLEMENTED("INST_WRITE64");
			case INST_I2F: UNIMPLEMENTED("INST_I2F");
			case INST_U2F: UNIMPLEMENTED("INST_U2F");
			case INST_F2I: UNIMPLEMENTED("INST_F2I");
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
	printf("segment .bss\n");
	printf("stack: resq EVM_STACK_CAPACITY\n");

	return 0;
}
