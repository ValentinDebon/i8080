#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#include "i8080/cpu.h"

struct i8080_args {
	const char *disassembly;
	unsigned run : 1;
};

static void
i8080_disassemble(const struct i8080_cpu *cpu, const char *filename) {
	FILE *filep = fopen(filename, "w");

	if(filep == NULL) {
		err(EXIT_FAILURE, "fopen %s", filename);
	}

	for(long i = 0; i < sizeof(cpu->memory); i++) {
		i8080_cpu_print_instruction(cpu, i, filep);
	}

	fclose(filep);
}

static void
i8080_usage(const char *i8080name) {
	fprintf(stderr, "usage: %s [-D <disassembly>] program\n", i8080name);
	exit(EXIT_FAILURE);
}

static struct i8080_args
i8080_parse_args(int argc, char **argv) {
	struct i8080_args args = {
		.disassembly = NULL,
		.run = 1,
	};
	int c;

	while((c = getopt(argc, argv, ":D:")) != -1) {
		switch(c) {
		case 'D':
			args.disassembly = optarg;
			args.run = 0;
			break;
		case '?':
			warnx("Invalid option -%c", optopt);
			i8080_usage(*argv);
		case ':':
			warnx("Missing option argument after -%c", optopt);
			i8080_usage(*argv);
		}
	}

	if(argc - optind != 1) {
		warnx("Expected one program file, found %d", argc - optind);
		i8080_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct i8080_args args = i8080_parse_args(argc, argv);
	char **argpos = argv + optind, ** const argend = argv + argc;

	while(argpos != argend) {
		const char * const filename = *argpos;
		struct i8080_cpu cpu;

		i8080_cpu_init_from_file(&cpu, filename, 0x100);

		if(args.disassembly != NULL) {
			i8080_disassemble(&cpu, args.disassembly);
		}

		if(args.run) {
			while(!cpu.stopped) {
				i8080_cpu_next(&cpu);
			}
		}

		i8080_cpu_deinit(&cpu);

		argpos++;
	}

	return EXIT_SUCCESS;
}
