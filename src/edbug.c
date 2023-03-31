#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "./edbug.h"

#define EVM_IMPLEMENTATION
#include "./evm.h"

#define INPUT_CAPACITY 32

static void usage(void) {
	fprintf(stderr, "usage: edb <executable>\n");
}

Edb_Err edb_state_init(Edb_State *state, const char *executable) {
	assert(state);
	assert(executable);

	state->code_file_name = sv_from_cstr(executable);
	evm_load_program_from_file(&state->evm, executable);
	evm_load_standard_natives(&state->evm);

	fprintf(stdout, "INFO : Loading debug symbols...\n");
    	return edb_load_symtab(state, arena_sv_concat2(&state->arena, executable, ".sym"));
}

Edb_Err edb_load_symtab(Edb_State *state, String_View symtab_file) {
	assert(state);

	String_View symtab = arena_slurp_file(&state->arena, symtab_file);
    	while (symtab.count > 0) {
		symtab = sv_trim_left(symtab);
		String_View raw_addr = sv_chop_by_delim(&symtab, '\t');
		symtab = sv_trim_left(symtab);
		String_View label_name = sv_chop_by_delim(&symtab, '\n');
		Inst_Addr addr = sv_to_u64(raw_addr);

        	if (addr < EVM_PROGRAM_CAPACITY) state->labels[addr] = label_name;
    	}

    return EDB_OK;
}

Edb_Err edb_run_command(Edb_State *state, String_View command_word, String_View arguments) {
	switch (*command_word.data) {
		case 'n': {
			Edb_Err err = edb_step_instr(state);
			if (err) return EXIT_FAILURE;

			printf("-> ");
			edb_print_instr(stdout, &state->evm.program[state->evm.ip]);
			printf("\n");
		} break;

		case 'i': {
			printf("ip = %lu \n", state->evm.ip);
		} break;

		case 'x': {
			arguments = sv_trim(arguments);
			String_View where_sv = sv_chop_by_delim(&arguments, ' ');
			String_View count_sv = arguments;

			Inst_Addr where = 0;
			if (edb_parse_label_or_addr(state, where_sv, &where) == EDB_FAIL) {
				fprintf(stderr, "ERR : Cannot parse address or label `"SV_Fmt"`\n", SV_Arg(where_sv));
				return EDB_FAIL;
			}

			Inst_Addr count = 0;
			if (edb_parse_label_or_addr(state, count_sv, &count) == EDB_FAIL) {
				fprintf(stderr, "ERR : Cannot parse address or label `"SV_Fmt"`\n", SV_Arg(count_sv));
				return EDB_FAIL;
			}

			for (Inst_Addr i = 0; i < count && where + i < EVM_MEMORY_CAPACITY; ++i) {
				printf("%02X ", state->evm.memory[where + i]);
			}
			printf("\n");
		} break;

		case 's': {
			evm_dump_stack(stdout, &state->evm);
		} break;

		case 'b': {
			Inst_Addr break_addr;
			String_View addr = sv_trim(arguments);

			if (edb_parse_label_or_addr(state, addr, &break_addr) == EDB_FAIL) {
				fprintf(stderr, "ERR : Cannot parse address or label\n");
				return EDB_FAIL;
			}

			edb_add_breakpoint(state, break_addr);
			fprintf(stdout, "INFO : Breakpoint set at %lu\n", break_addr);
		} break;

		case 'd': {
			Inst_Addr break_addr;
			String_View addr = sv_trim(arguments);

			if (edb_parse_label_or_addr(state, addr, &break_addr) == EDB_FAIL) {
				fprintf(stderr, "ERR : Cannot parse address or label\n");
				return EDB_FAIL;
			}

			edb_delete_breakpoint(state, break_addr);
			fprintf(stdout, "INFO : Deleted breakpoint at %lu\n", break_addr);
		} break;

		case 'r':
			if (!state->evm.halt) {
				// TODO: Reset evm and restart program
				fprintf(stderr, "ERR : Program is already running\n");
			}

			state->evm.halt = 0;
		// fall through

		case 'c':
			if (edb_continue(state)) return EXIT_FAILURE;
		break;

		case EOF:
		case 'q':
			return EDB_EXIT;

		case 'h':
			printf("r - run program\n"
				"n - next instruction\n"
				"c - continue program execution\n"
				"s - stack dump\n"
				"i - instruction pointer\n"
				"x - inspect the memory\n"
				"b - set breakpoint at address or label\n"
				"d - destroy breakpoint at address or label\n"
				"q - quit\n");
		break;

		default:
			fprintf(stderr, "?\n");
		break;
	}

	 return EDB_OK;
}

Edb_Err edb_step_instr(Edb_State *state) {
	assert(state);

    	if (state->evm.halt) {
        	fprintf(stderr, "ERR : Program is not being run\n");
        	return EDB_OK;
    	}

    	Err err = evm_execute_inst(&state->evm);
    	if (!err)
        	return EDB_OK;
    	else
        	return edb_fault(state, err);
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
                		fprintf(stdout, " label '"SV_Fmt"'", SV_Arg(state->labels[state->evm.ip]));

            		fprintf(stdout, "\n");
            		bp->is_broken = 1;

            		return EDB_OK;
        	}

        	bp->is_broken = 0;

        	Err err = evm_execute_inst(&state->evm);
        	if (err) return edb_fault(state, err);
    	} while (!state->evm.halt);

    	printf("Program halted.\n");

    	return EDB_OK;
}

Edb_Err edb_find_addr_of_label(Edb_State *state, String_View name, Inst_Addr *out) {
    	assert(state);
    	assert(out);

    	for (Inst_Addr i = 0; i < EVM_PROGRAM_CAPACITY; ++i) {
        	if (state->labels[i].data && sv_eq(state->labels[i], name)) {
            		*out = i;
            		return EDB_OK;
        	}
    	}

    	return EDB_FAIL;
}

Edb_Err edb_parse_label_or_addr(Edb_State *st, String_View addr, Inst_Addr *out) {
    	assert(st);
    	assert(out);
	if (addr.count == 0) return EDB_FAIL;

    	char *endptr = NULL;

    	*out = strtoull(addr.data, &endptr, 10);
    	if (endptr != addr.data + addr.count) {
        	if (edb_find_addr_of_label(st, addr, out) == EDB_FAIL) return EDB_FAIL;
    	}

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

Edb_Err edb_fault(Edb_State *state, Err err) {
	assert(state);

    	fprintf(stderr, "%s at %lu (INSTR: ", err_as_cstr(err), state->evm.ip);
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

	// NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
	static Edb_State state = { 0 };
    	state.evm.halt = 1;

    	printf("EDB - The birtual machine debugger.\nType 'h' and enter for a quick help\n");
    	if (edb_state_init(&state, argv[1]) == EDB_FAIL) {
        	fprintf(stderr, "FATAL : Unable to initialize the debugger: %s\n", strerror(errno));
    	}

    	char previous_command[INPUT_CAPACITY] = { 0 };
    	while (!feof(stdin)) {
		printf("(edb) ");
        	char input_buf[INPUT_CAPACITY] = { 0 };
        	fgets(input_buf, INPUT_CAPACITY, stdin);
		String_View input_sv = sv_from_cstr(input_buf);
            	String_View control_word = sv_trim(sv_chop_by_delim(&input_sv, ' '));
		if (control_word.count == 0) {
			if (*previous_command == '\0') {
				fprintf(stderr, "ERR : No previous command\n");
				continue;
			}
			input_sv = sv_from_cstr(previous_command);
            		control_word = sv_trim(sv_chop_by_delim(&input_sv, ' '));
		}

		Edb_Err err = edb_run_command(&state, control_word, input_sv);
		if ((err == EDB_OK) && (*input_buf != '\n') ) {
			memcpy(previous_command, input_buf, sizeof(char) * INPUT_CAPACITY);
		} else if (err == EDB_EXIT) {
			printf("Bye\n");
            		return EXIT_SUCCESS;
		}
	}
}

