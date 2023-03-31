#include <stdio.h>
#include <stdlib.h>

#define EVM_IMPLEMENTATION
#include "./evm.h"

static void usage(FILE *f) {
		fprintf(f, "Usage: easm2nasm <input.easm> <output.asm>\n");
}

static char *shift(int *argc, char ***argv) {
	assert(*argc > 0);
	char *result = **argv;
	*argv += 1;
	*argc -= 1;
	return result;
}

int main(int argc, char **argv) {
	shift(&argc, &argv);        // skip the program

	if (argc == 0) {
		usage(stderr);
		fprintf(stderr, "ERROR: no input provided\n");
		exit(1);
	}

	const char *input_file_path = shift(&argc, &argv);

    	if (argc == 0) {
        	usage(stderr);
        	fprintf(stderr, "ERROR: no output provided.\n");
        	exit(1);
    	}
    	const char *output_file_path = shift(&argc, &argv);

	// NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
	static EASM easm = { 0 };
	easm_translate_source(&easm, sv_from_cstr(input_file_path));

    	FILE *output = fopen(output_file_path, "wb");
    	if (output == NULL) {
        	fprintf(stderr, "ERROR: could not open file %s: %s\n", output_file_path, strerror(errno));
        	exit(1);
    	}

	fprintf(output, "BITS 64\n");
	fprintf(output, "%%define EVM_STACK_CAPACITY %d\n", EVM_STACK_CAPACITY);
	fprintf(output, "%%define EVM_WORD_SIZE %d\n", EVM_WORD_SIZE);
	fprintf(output, "%%define STDOUT 1\n");
	fprintf(output, "%%define SYS_EXIT 60\n");
	fprintf(output, "%%define SYS_WRITE 1\n");
	fprintf(output, "segment .text\n");
	fprintf(output, "global _start\n");

	size_t jmp_if_escape_count = 0;
	for (size_t i = 0; i < easm.program_size; ++i) {
		Inst inst = easm.program[i];

		if (i == easm.entry) {
	    		fprintf(output, "_start:\n");
		}

		for (size_t j = 0; j < EASM_BINDINGS_CAPACITY; ++j) {
            		if (easm.bindings[j].kind != BINDING_LABEL) continue;

            		if (easm.bindings[j].value.as_u64 == i) {
                		fprintf(output, "\n;; "SV_Fmt":\n", SV_Arg(easm.bindings[j].name));
                		break;
            		}
        	}

		fprintf(output, "inst_%zu:\n", i);
		switch (inst.type) {
			case INST_NOP: UNIMPLEMENTED("INST_NOP");

			case INST_PUSH: {
				fprintf(output, "\t;; push %lu\n", inst.operand.as_u64);
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tmov rax, 0x%lx\n", inst.operand.as_u64);
				fprintf(output, "\tmov QWORD [rsi], rax\n");
				fprintf(output, "\tadd QWORD [stack_top], EVM_WORD_SIZE\n");
			} break;

			case INST_DROP: {
				fprintf(output, "\t;; drop\n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
	    			fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
	    			fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_DUP: {
				fprintf(output, "\t;; dup %lu\n", inst.operand.as_u64);
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tmov rdi, rsi\n");
				fprintf(output, "\tsub rdi, EVM_WORD_SIZE * (%lu + 1)\n", inst.operand.as_u64);
				fprintf(output, "\tmov rax, [rdi]\n");
				fprintf(output, "\tmov [rsi], rax\n");
				fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_SWAP: {
				fprintf(output, "\t;; swap %lu\n", inst.operand.as_u64);
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rdi, rsi\n");
				fprintf(output, "\tsub rdi, EVM_WORD_SIZE * %lu\n", inst.operand.as_u64);
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\tmov rbx, [rdi]\n");
				fprintf(output, "\tmov [rdi], rax\n");
				fprintf(output, "\tmov [rsi], rbx\n");
			} break;

			case INST_PLUSI: {
				fprintf(output, "\t;; plusi \n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rbx, [rsi]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\tadd rax, rbx\n");
				fprintf(output, "\tmov [rsi], rax\n");
				fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_MINUSI: {
				fprintf(output, "\t;; minusi \n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rbx, [rsi]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\tsub rax, rbx\n");
				fprintf(output, "\tmov [rsi], rax\n");
				fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_MULTI: {
				fprintf(output, "\t;; multi\n");
				fprintf(output, "\tmov r11, [stack_top]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], r11\n");
				fprintf(output, "\tmov rax, [r11]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rbx, [r11]\n");
				fprintf(output, "\timul rax, rbx\n");
				fprintf(output, "\tmov [r11], rax\n");
			} break;

			case INST_MULTU: UNIMPLEMENTED("INST_MULTU");

			case INST_DIVI: {
	    			fprintf(output, "\t;; divi\n");
	    			fprintf(output, "\tmov rsi, [stack_top]\n");
	    			fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
	    			fprintf(output, "\tmov rbx, [rsi]\n");
	    			fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
	    			fprintf(output, "\tmov rax, [rsi]\n");
	    			fprintf(output, "\txor rdx, rdx\n");
	    			fprintf(output, "\tidiv rbx\n");
	    			fprintf(output, "\tmov [rsi], rax\n");
	    			fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
	    			fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_DIVU: {
				fprintf(output, "\t;; divu\n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rbx, [rsi]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\txor rdx, rdx\n");
				fprintf(output, "\tdiv rbx\n");
				fprintf(output, "\tmov [rsi], rax\n");
				fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_MODI: {
				fprintf(output, "\t;; modi\n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rbx, [rsi]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\txor rdx, rdx\n");
				fprintf(output, "\tidiv rbx\n");
				fprintf(output, "\tmov [rsi], rdx\n");
				fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_MODU: {
				fprintf(output, "\t;; modu\n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rbx, [rsi]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\txor rdx, rdx\n");
				fprintf(output, "\tdiv rbx\n");
				fprintf(output, "\tmov [rsi], rdx\n");
				fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_PLUSF: {
				fprintf(output, "\t;; plusf\n");
				fprintf(output, "\tmov r11, [stack_top]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], r11\n");
				fprintf(output, "\tmovsd xmm0, [r11]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmovsd xmm1, [r11]\n");
				fprintf(output, "\taddsd xmm1, xmm0\n");
				fprintf(output, "\tmovsd [r11], xmm1\n");
			} break;

			case INST_MINUSF: {
				fprintf(output, "\t;; minusf\n");
				fprintf(output, "\tmov r11, [stack_top]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], r11\n");
				fprintf(output, "\tmovsd xmm0, [r11]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmovsd xmm1, [r11]\n");
				fprintf(output, "\tsubsd xmm1, xmm0\n");
				fprintf(output, "\tmovsd [r11], xmm1\n");
			} break;

			case INST_MULTF: {
				fprintf(output, "\t;; multf\n");
				fprintf(output, "\tmov r11, [stack_top]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], r11\n");
				fprintf(output, "\tmovsd xmm0, [r11]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmovsd xmm1, [r11]\n");
				fprintf(output, "\tmulsd xmm1, xmm0\n");
				fprintf(output, "\tmovsd [r11], xmm1\n");
			} break;

			case INST_DIVF: {
				fprintf(output, "\t;; divf\n");
				fprintf(output, "\tmov r11, [stack_top]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], r11\n");
				fprintf(output, "\tmovsd xmm0, [r11]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmovsd xmm1, [r11]\n");
				fprintf(output, "\tdivsd xmm1, xmm0\n");
				fprintf(output, "\tmovsd [r11], xmm1\n");
			} break;

			case INST_JMP: {
				fprintf(output, "\t;; jmp\n");
				fprintf(output, "\tmov rdi, inst_map\n");
				fprintf(output, "\tadd rdi, EVM_WORD_SIZE * %lu\n", inst.operand.as_u64);
				fprintf(output, "\tjmp [rdi]\n");
			} break;

			case INST_JMP_IF: {
				fprintf(output, "\t;; jmp_if %lu\n", inst.operand.as_u64);
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
				fprintf(output, "\tcmp rax, 0\n");
				fprintf(output, "\tje jmp_if_escape_%zu\n", jmp_if_escape_count);
				fprintf(output, "\tmov rdi, inst_map\n");
				fprintf(output, "\tadd rdi, EVM_WORD_SIZE * %lu\n", inst.operand.as_u64);
				fprintf(output, "\tjmp [rdi]\n");
				fprintf(output, "jmp_if_escape_%zu:\n", jmp_if_escape_count);
				jmp_if_escape_count += 1;
			} break;

			case INST_RET: {
				fprintf(output, "\t;; ret\n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\tmov rbx, EVM_WORD_SIZE\n");
				fprintf(output, "\tmul rbx\n");
				fprintf(output, "\tadd rax, inst_map\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
				fprintf(output, "\tjmp [rax]\n");
			} break;

			case INST_CALL: {
				fprintf(output, "\t;; call\n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tmov QWORD [rsi], %zu\n", i + 1);
				fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
				fprintf(output, "\tmov rdi, inst_map\n");
				fprintf(output, "\tadd rdi, EVM_WORD_SIZE * %lu\n", inst.operand.as_u64);
				fprintf(output, "\tjmp [rdi]\n");
			} break;

			case INST_NATIVE: {
				if (inst.operand.as_u64 == 0) {
					fprintf(output, "\t;; native write\n");
					fprintf(output, "\tmov r11, [stack_top]\n");
					fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
					fprintf(output, "\tmov rdx, [r11]\n");
					fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
					fprintf(output, "\tmov rsi, [r11]\n");
					fprintf(output, "\tadd rsi, memory\n");
					fprintf(output, "\tmov rdi, STDOUT\n");
					fprintf(output, "\tmov rax, SYS_WRITE\n");
					fprintf(output, "\tmov [stack_top], r11\n");
					fprintf(output, "\tsyscall\n");
				} else UNIMPLEMENTED("Unsupported native function");
			} break;

			case INST_NOT: {
				fprintf(output, "\t;; not\n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\tcmp rax, 0\n");
				fprintf(output, "\tmov rax, 0\n");
				fprintf(output, "\tsetz al\n");
				fprintf(output, "\tmov [rsi], rax\n");
	    		} break;

			case INST_EQI: {
				fprintf(output, "\t;; eqi\n");
				fprintf(output, "\tmov rsi, [stack_top]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rbx, [rsi]\n");
				fprintf(output, "\tsub rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [rsi]\n");
				fprintf(output, "\tcmp rax, rbx\n");
				fprintf(output, "\tmov rax, 0\n");
				fprintf(output, "\tsetz al\n");
				fprintf(output, "\tmov [rsi], rax\n");
				fprintf(output, "\tadd rsi, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov [stack_top], rsi\n");
			} break;

			case INST_GEI: {
	    			fprintf(output, "\t;; FIXME: gei\n");
			} break;

			case INST_GTI: {
	    			fprintf(output, "\t;; FIXME: gti\n");
			} break;

			case INST_LEI: {
	    			fprintf(output, "\t;; FIXME: lei\n");
			} break;

			case INST_LTI: UNIMPLEMENTED("INST_LTI");
			case INST_NEI: UNIMPLEMENTED("INST_NEI");

			case INST_EQU: {
	    			fprintf(output, "\t;; FIXME: equ\n");
			} break;

			case INST_GEU: UNIMPLEMENTED("INST_GEU");
			case INST_GTU: UNIMPLEMENTED("INST_GTU");
			case INST_LEU: UNIMPLEMENTED("INST_LEU");
			case INST_LTU: UNIMPLEMENTED("INST_LTU");
			case INST_NEU: UNIMPLEMENTED("INST_NEU");
			case INST_EQF: UNIMPLEMENTED("INST_EQF");

			case INST_GEF: {
			    	fprintf(output, "\t;; FIXME: gef\n");
			} break;

			case INST_GTF: {
			    	fprintf(output, "\t;; FIXME: gtf\n");
			} break;

			case INST_LEF: {
			    	fprintf(output, "\t;; FIXME: lef\n");
			} break;

			case INST_LTF: {
			    	fprintf(output, "\t;; FIXME: ltf\n");
			} break;

			case INST_NEF: UNIMPLEMENTED("INST_NEF");

			case INST_ANDB: {
	    			fprintf(output, "\t;; FIXME: andb\n");
			} break;

			case INST_ORB: UNIMPLEMENTED("INST_ORB");

			case INST_XOR: {
	    			fprintf(output, "\t;; FIXME: xor\n");
			} break;

			case INST_SHR: UNIMPLEMENTED("INST_SHR");
			case INST_SHL: UNIMPLEMENTED("INST_SHL");
			case INST_NOTB: UNIMPLEMENTED("INST_NOTB");

			case INST_READ8: {
				fprintf(output, "\t;; read8\n");
				fprintf(output, "\tmov r11, [stack_top]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rsi, [r11]\n");
				fprintf(output, "\tadd rsi, memory\n");
				fprintf(output, "\txor rax, rax\n");
				fprintf(output, "\tmov al, BYTE [rsi]\n");
				fprintf(output, "\tmov [r11], rax\n");
			} break;

			case INST_READ16: UNIMPLEMENTED("INST_READ16");
			case INST_READ32: UNIMPLEMENTED("INST_READ32");
			case INST_READ64: UNIMPLEMENTED("INST_READ64");

			case INST_WRITE8: {
				fprintf(output, "\t;; write8\n");
				fprintf(output, "\tmov r11, [stack_top]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rax, [r11]\n");
				fprintf(output, "\tsub r11, EVM_WORD_SIZE\n");
				fprintf(output, "\tmov rsi, [r11]\n");
				fprintf(output, "\tadd rsi, memory\n");
				fprintf(output, "\tmov BYTE [rsi], al\n");
				fprintf(output, "\tmov [stack_top], r11\n");
			} break;

			case INST_WRITE16: UNIMPLEMENTED("INST_WRITE16");
			case INST_WRITE32: UNIMPLEMENTED("INST_WRITE32");
			case INST_WRITE64: UNIMPLEMENTED("INST_WRITE64");

			case INST_I2F: {
	    			fprintf(output, "\t;; FIXME: i2f\n");
			} break;

			case INST_U2F: UNIMPLEMENTED("INST_U2F");

			case INST_F2I: {
			    	fprintf(output, "\t;; FIXME: f2i\n");
			} break;

			case INST_F2U: UNIMPLEMENTED("INST_F2U");

			case INST_HALT: {
				fprintf(output, "\t;; halt\n");
				fprintf(output, "\tmov rax, SYS_EXIT\n");
				fprintf(output, "\tmov rdi, 0\n");
				fprintf(output, "\tsyscall\n");
			} break;

			case EASM_NUMBER_OF_INSTS:
			default: UNREACHABLE("NOT EXISTING INST_TYPE");
		}
	}

	fprintf(output, "\tret\n");
	fprintf(output, "segment .data\n");
	fprintf(output, "stack_top: dq stack\n");
	fprintf(output, "inst_map:\n");
#define ROW_SIZE 5
#define ROW_COUNT(size) ((size + ROW_SIZE - 1) / ROW_SIZE)
#define INDEX(row, col) ((row) * ROW_SIZE + (col))
    	for (size_t row = 0; row < ROW_COUNT(easm.program_size); ++row) {
        	fprintf(output, "  dq");
        	for (size_t col = 0; col < ROW_SIZE && INDEX(row, col) < easm.program_size; ++col) {
            		fprintf(output, " inst_%zu,", INDEX(row, col));
        	}
        	fprintf(output, "\n");
    	}
	fprintf(output, "\n");
    	fprintf(output, "memory:\n");
    	for (size_t row = 0; row < ROW_COUNT(easm.memory_size); ++row) {
		fprintf(output, "\tdb");
        	for (size_t col = 0; col < ROW_SIZE && INDEX(row, col) < easm.memory_size; ++col) {
            		fprintf(output, " %u,", easm.memory[INDEX(row, col)]);
		}
		fprintf(output, "\n");
    	}
    	// TODO: warning: uninitialized space declared in non-BSS section `.data': zeroing
    	//   Is it possible to get rid of it? Not really suppress it, but just let nasm know
    	//   that I know what I'm doing.
    	fprintf(output, "\ttimes %zu db 0", EVM_MEMORY_CAPACITY - easm.memory_size);
#undef ROW_SIZE
#undef ROW_COUNT
    	fprintf(output, "\n");
	fprintf(output, "segment .bss\n");
	fprintf(output, "stack: resq EVM_STACK_CAPACITY\n");

	fclose(output);
	return 0;
}
