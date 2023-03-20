#define EVM_IMPLEMENTATION
#include "./evm.h"

static char *shift(int *argc, char ***argv) {
	assert(*argc > 0);
	char *result = **argv;
	*argv += 1;
	*argc -= 1;
	return result;
}

static void usage(FILE *stream, const char *program) {
	fprintf(stream, "Usage: %s [-g] <input.easm> <output.bm>\n", program);
}

int main(int argc, char **argv) {
	int have_symbol_table = 0;

	const char *program = shift(&argc, &argv);
	if (argc == 0) {
		usage(stderr, program);
		fprintf(stderr, "ERROR: expected input\n");
        	exit(1);
	}

	const char *input_file_path = shift(&argc, &argv);
	if (!strcmp(input_file_path, "-g")) {
        	have_symbol_table = 1;
        	input_file_path = shift(&argc, &argv);
    	}

	if (argc == 0) {
        	usage(stderr, program);
        	fprintf(stderr, "ERROR: expected output\n");
        	exit(1);
    	}

	const char *output_file_path = shift(&argc, &argv);

	// NOTE: The structure might be quite big due its arena. Better allocate it in the static memory.
	static EASM easm = { 0 };

	easm_translate_source(&easm, cstr_as_sv(input_file_path));

	if (!easm.has_entry) {
		fprintf(stderr, "%s: ERROR: entry point for EVM not provided. Use preprocessor directive #entry to provide the entry point\n", input_file_path);
		exit(1);
	}

	easm_save_to_file(&easm, output_file_path);

	if (have_symbol_table) {
		const char *sym_file_name = arena_cstr_concat2(&easm.arena, output_file_path, ".sym");
		FILE *symbol_file = fopen(sym_file_name, "w");
		if (!symbol_file) {
			fprintf(stderr, "ERROR: Unable to open symbol table file\n");
			return EXIT_FAILURE;
		}

		/*
		* Note: This will dump out *ALL* symbols, no matter whether
		* they are jump labels or not. However, since the
		* preprocessor runs before the jump mark resolution, all the
		* labels are allocated in a way that enables us to just
		* overwrite prerocessor labels with a value equal to the
		* address of a jump label.
		*
		*/
		for (size_t i = 0; i < easm.labels_size; ++i) {
			fprintf(symbol_file, "%lu \t%.*s\n", easm.labels[i].word.as_u64, (int)easm.labels[i].name.count, easm.labels[i].name.data);
		}
		fclose(symbol_file);
	}

	return 0;
}

