#ifndef EDB_H_
#define EDB_H_

#include "./evm.h"

typedef enum {
	EDB_OK = 0,
	EDB_FAIL = 1,
	EDB_EXIT,
} Edb_Err;

typedef struct {
	int is_enabled;
	int id;
	int is_broken;
} Edb_Breakpoint;

typedef struct {
	EVM evm;
	String_View code_file_name;
	Edb_Breakpoint breakpoints[EVM_PROGRAM_CAPACITY];
	String_View labels[EVM_PROGRAM_CAPACITY];
	Arena arena;
} Edb_State;

Edb_Err edb_state_init(Edb_State *state, const char *executable);
Edb_Err edb_load_symtab(Edb_State *state, String_View symtab_file);
Edb_Err edb_step_instr(Edb_State *state);
Edb_Err edb_continue(Edb_State *state);
Edb_Err edb_find_addr_of_label(Edb_State *state, const char *name, Inst_Addr *out);
Edb_Err edb_parse_label_or_addr(Edb_State *st, const char *in, Inst_Addr *out);
void edb_print_instr(FILE *f, Inst *i);
void edb_add_breakpoint(Edb_State *state, Inst_Addr addr);
void edb_delete_breakpoint(Edb_State *state, Inst_Addr addr);
Edb_Err edb_fault(Edb_State *state, Err err);

#endif // EDB_H
