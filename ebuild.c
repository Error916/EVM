#define EBUILD_IMPLEMENTAION
#include "./ebuild.h"

#define CFLAGS "-pedantic", "-Wall", "-Wextra", "-Werror", "-Wfatal-errors", "-Wswitch-enum", "-Wmissing-prototypes", "-Wconversion", "-Ofast", "-flto", "-march=native", "-pipe", "-fno-strict-aliasing"

const char *toolchian[] = {
	"easm", "evmi", "evmr", "deasm", "edbug", "easm2nasm"
};

void build_toolchain(void) {
	MKDIRS("build", "bin");
	FOREACH_ARRAY(const char *, tool, toolchian, {
		CMD("gcc", CFLAGS, "-o",
			PATH("build", "bin", tool),
			PATH("src", CONCAT(tool, ".c")));
	});
}

void build_examples(void) {
	MKDIRS("build", "examples");
	FOREACH_FILE_IN_DIR(example, "examples", {
		size_t n = strlen(example);
		if (*example != '.') {
			assert(n >= 4);
			if (strcmp(example + n - 4, "easm") == 0) {
				const char *example_base = NOEXT(example);
				CMD(PATH("build", "bin", "easm"), "-g",
					PATH("examples", example),
					PATH("build", "examples", CONCAT(example_base, ".evm")));
			}
		}
	});

}

void build_x86_64_example(const char *example) {
    	CMD(PATH("build", "bin", "easm2nasm"),
        	PATH("examples", CONCAT(example, ".easm")),
        	PATH("build", "examples", CONCAT(example, ".asm")));

    	CMD("nasm", "-felf64", "-F", "dwarf", "-g",
        	PATH("build", "examples", CONCAT(example, ".asm")),
        	"-o",
        	PATH("build", "examples", CONCAT(example, ".o")));

    	CMD("ld",
        	"-o", PATH("build", "examples", CONCAT(example, ".exe")),
        	PATH("build", "examples", CONCAT(example, ".o")));
}

void build_x86_64_examples(void) {
    	build_x86_64_example("123i");
    	build_x86_64_example("fib");
}

void run_tests(void) {
	FOREACH_FILE_IN_DIR(example, "examples", {
		size_t n = strlen(example);
		if (*example != '.') {
			assert(n >= 4);
			if (strcmp(example + n - 4, "easm") == 0) {
				const char *example_base = NOEXT(example);
				CMD(PATH("build", "bin", "evmr"),
					"-p", PATH("build", "examples", CONCAT(example_base, ".evm")),
					"-eo", PATH("test", "examples", CONCAT(example_base, ".expected.out")));
			}
		}
	});
}

void record_tests(void) {
    	FOREACH_FILE_IN_DIR(example, "examples", {
        	size_t n = strlen(example);
        	if (*example != '.') {
        		assert(n >= 4);
            		if (strcmp(example + n - 4, "easm") == 0) {
                		const char *example_base = NOEXT(example);
                		CMD(PATH("build", "bin", "evmr"),
                    			"-p", PATH("build", "examples", CONCAT(example_base, ".evm")),
                    			"-ao", PATH("test", "examples", CONCAT(example_base, ".expected.out")));
            		}
        	}
    });
}

void print_help(FILE *stream) {
	fprintf(stream, "./nobuild          - Build toolchain and examples\n");
	fprintf(stream, "./nobuild test     - Run the tests\n");
	fprintf(stream, "./nobuild record   - Capture the current output of examples as the expected on for the tests\n");
	fprintf(stream, "./nobuild help     - Show this help message\n");
	}

int main(int argc, char **argv) {
	shift(&argc, &argv);

	const char *subcommand = NULL;

	if (argc > 0) {
		subcommand = shift(&argc, &argv);
	}

	if (subcommand != NULL && strcmp(subcommand, "help") == 0) {
		print_help(stdout);
		exit(0);
	}

	RM("build");

	build_toolchain();
	build_examples();
#ifdef __linux__
    	build_x86_64_examples();
#endif // __linux__

	if (subcommand) {
        	if (strcmp(subcommand, "test") == 0) {
            		run_tests();
        	} else if (strcmp(subcommand, "record") == 0) {
            		record_tests();
        	} else {
            		print_help(stderr);
            		fprintf(stderr, "[ERROR] unknown subcommand `%s`\n", subcommand);
            		exit(1);
        	}
    	}

	return 0;
}
