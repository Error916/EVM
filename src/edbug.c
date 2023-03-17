#include <assert.h>
#include <fcntl.h>
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__NetBSD__) \
    || defined(__OpenBSD__) || defined(__DragonFly__)
#include <limits.h>
#else
#define PATH_MAX 4096
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "./edbug.h"

#define EVM_IMPLEMENTATION
#include "./evm.h"

static void usage(void) {
	fprintf(stderr, "usage: edb <executable>\n");
}

Edb_Err edb_state_init(Edb_State *state, const char *executable) {
	assert(state);
	assert(executable);

	state->code_file_name = cstr_as_sv(executable);
	evm_load_program_from_file(&state->evm, executable);

	char buf[PATH_MAX];
    	memcpy(buf, executable, strlen(executable));
    	memcpy(buf + strlen(executable), ".sym", 5);

    	if (access(buf, R_OK) == 0) {
        	fprintf(stdout, "INFO : Loading debug symbols...\n");
        	return edb_load_symtab(state, buf);
    	}

	return EDB_OK;
}

Edb_Err edb_load_symtab(Edb_State *state, const char *symtab_file) {
	assert(state);
    	assert(symtab_file);

    	String_View symtab;
	if (edb_mmap_file(symtab_file, &symtab) == EDB_FAIL) return EDB_FAIL;
    	while (symtab.count > 0) {
		symtab = sv_trim_left(symtab);
		String_View raw_addr   = sv_chop_by_delim(&symtab, '\t');
		symtab = sv_trim_left(symtab);
		String_View label_name = sv_chop_by_delim(&symtab, '\n');
		Inst_Addr addr = (Inst_Addr)sv_to_int(raw_addr);

        	if (addr < EVM_PROGRAM_CAPACITY) state->labels[addr] = label_name;
    	}

    return EDB_OK;
}

Edb_Err edb_step_instr(Edb_State *state) {
	assert(state);

    	if (state->evm.halt) {
        	fprintf(stderr, "ERR : Program is not being run\n");
        	return EDB_OK;
    	}

    	Trap trap = evm_execute_inst(&state->evm);
    	if (!trap)
        	return EDB_OK;
    	else
        	return edb_fault(state, trap);
}

Edb_Err edb_continue(Edb_State *state) {
    	assert(state);

    	if (state->evm.halt) {
        	fprintf(stderr, "ERR : Program is not being run\n");
        	return EDB_OK;
    	}

    	do {
        	Edb_Breakpoint *bp = &state->breakpoints[state->evm.ip];
        	if (!bp->is_broken && bp->is_enabled) {
            		fprintf(stdout, "Hit breakpoint at %lu", state->evm.ip);
            		if (state->labels[state->evm.ip].data)
                		fprintf(stdout, " label '%.*s'", (int)state->labels[state->evm.ip].count, state->labels[state->evm.ip].data);

            		fprintf(stdout, "\n");
            		bp->is_broken = 1;

            		return EDB_OK;
        	}

        	bp->is_broken = 0;

        	Trap trap = evm_execute_inst(&state->evm);
        	if (trap) return edb_fault(state, trap);
    	} while (!state->evm.halt);

    	printf("Program halted.\n");

    	return EDB_OK;
}

Edb_Err edb_find_addr_of_label(Edb_State *state, const char *name, Inst_Addr *out) {
    	assert(state);
    	assert(name);
    	assert(out);

    	String_View _name = sv_trim_right(cstr_as_sv(name));
    	for (Inst_Addr i = 0; i < EVM_PROGRAM_CAPACITY; ++i) {
        	if (state->labels[i].data && sv_eq(state->labels[i], _name)) {
            		*out = i;
            		return EDB_OK;
        	}
    	}

    	return EDB_FAIL;
}

Edb_Err edb_parse_label_or_addr(Edb_State *st, const char *in, Inst_Addr *out) {
    	assert(st);
    	assert(in);
    	assert(out);

    	char *endptr = NULL;
    	size_t len = strlen(in);

    	*out = strtoull(in, &endptr, 10);
    	if (endptr != in + len) {
        	if (edb_find_addr_of_label(st, in, out) == EDB_FAIL) return EDB_FAIL;
    	}

    	return EDB_OK;
}

Edb_Err edb_mmap_file(const char *path, String_View *out) {
   	assert(path);
   	assert(out);

    	int fd;

    	if ((fd = open(path, O_RDONLY)) < 0) return EDB_FAIL;

    	struct stat stat_buf;
    	if (fstat(fd, &stat_buf) < 0) return EDB_FAIL;

    	char *content = mmap(NULL, (size_t)stat_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    	if (content == MAP_FAILED) return EDB_FAIL;

    	out->data = content;
    	out->count = (size_t)stat_buf.st_size;

    	close(fd);

    	return EDB_OK;
}

void edb_print_instr(FILE *f, Inst *i) {
	fprintf(f, "%s ", inst_name(i->type));
    	if (inst_has_operand(i->type))
        	fprintf(f, "%ld", i->operand.as_i64);
}

void edb_add_breakpoint(Edb_State *state, Inst_Addr addr) {
    	assert(state);

    	if (addr > state->evm.program_size) {
        	fprintf(stderr, "ERR : Symbol out of program\n");
        	return;
    	}

    	if (addr > EVM_PROGRAM_CAPACITY) {
        	fprintf(stderr, "ERR : Symbol out of memory\n");
        	return;
    	}

    	if (state->breakpoints[addr].is_enabled) {
        	fprintf(stderr, "ERR : Breakpoint already set\n");
        	return;
    	}

    	state->breakpoints[addr].is_enabled = 1;
}

void edb_delete_breakpoint(Edb_State *state, Inst_Addr addr) {
    	assert(state);

    	if (addr > state->evm.program_size) {
        	fprintf(stderr, "ERR : Symbol out of program\n");
        	return;
    	}

    	if (addr > EVM_PROGRAM_CAPACITY) {
        	fprintf(stderr, "ERR : Symbol out of memory\n");
        	return;
    	}

    	if (!state->breakpoints[addr].is_enabled) {
		fprintf(stderr, "ERR : No such breakpoint\n");
        	return;
    	}

    	state->breakpoints[addr].is_enabled = 0;
}

Edb_Err edb_fault(Edb_State *state, Trap trap) {
	assert(state);

    	fprintf(stderr, "%s at %lu (INSTR: ", trap_as_cstr(trap), state->evm.ip);
    	edb_print_instr(stderr, &state->evm.program[state->evm.ip]);
    	fprintf(stderr, ")\n");
    	state->evm.halt = 1;
    	return EDB_OK;
}

int main(int argc, char **argv) {
    	if (argc < 2) {
		usage();
        	return EXIT_FAILURE;
    	}

	Edb_State state = {0};
    	state.evm.halt = 1;

    	printf("EDB - The birtual machine debugger.\nType 'h' and enter for a quick help\n");
    	if (edb_state_init(&state, argv[1]) == EDB_FAIL) {
        	fprintf(stderr, "FATAL : Unable to initialize the debugger: %s\n", strerror(errno));
    	}

    	while (1) {
		printf("(edb) ");
        	char input_buf[32];
        	fgets(input_buf, 32, stdin);

        	switch (*input_buf) {
        		case 'n': {
            			Edb_Err err = edb_step_instr(&state);
            			if (err) return EXIT_FAILURE;

            			printf("-> ");
            			edb_print_instr(stdout, &state.evm.program[state.evm.ip]);
            			printf("\n");
        		} break;

			case 'i': {
            			printf("ip = %lu \n", state.evm.ip);
        		} break;

        		case 's': {
            			evm_dump_stack(stdout, &state.evm);
        		} break;

			case 'b': {
            			// TODO: `b 0` in edb results in "ERR : Cannot parse address or labels"
            			char *addr = input_buf + 2;
            			Inst_Addr break_addr;

            			if (edb_parse_label_or_addr(&state, addr, &break_addr) == EDB_FAIL) {
            		    		fprintf(stderr, "ERR : Cannot parse address or labels\n");
                			continue;
            			}

            			edb_add_breakpoint(&state, break_addr);
            			fprintf(stdout, "INFO : Breakpoint set at %lu\n", break_addr);
        		} break;

			case 'd': {
            			char *addr = input_buf + 2;
            			Inst_Addr break_addr;

            			if (edb_parse_label_or_addr(&state, addr, &break_addr) == EDB_FAIL) {
                			fprintf(stderr, "ERR : Cannot parse address or labels\n");
                			continue;
            			}

            			edb_delete_breakpoint(&state, break_addr);
            			fprintf(stdout, "INFO : Deleted breakpoint at %lu\n", break_addr);
        		} break;

        		case 'r':
            			if (!state.evm.halt) {
                			// TODO: Reset evm and restart program
                			fprintf(stderr, "ERR : Program is already running\n");
            			}

            			state.evm.halt = 0;
        		// fall through

			case 'c':
            			if (edb_continue(&state)) return EXIT_FAILURE;
        		break;

			case EOF:
        		case 'q':
            			return EXIT_SUCCESS;

        		case 'h':
            			printf("r - run program\n"
                   			"n - next instruction\n"
                   			"c - continue program execution\n"
                   			"s - stack dump\n"
                   			"i - instruction pointer\n"
                   			"b - set breakpoint at address or label\n"
                   			"d - destroy breakpoint at address or label\n"
                   			"q - quit\n");
        		break;

			default:
            			fprintf(stderr, "?\n");
        		break;
        	}
    	}

    	return EXIT_SUCCESS;
}
