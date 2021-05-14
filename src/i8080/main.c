#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "i8080/cpu.h"

struct i8080_args {
};

static void
cpm_input(struct i8080_cpu *cpu, uint8_t device) {
}

static void
cpm_output(struct i8080_cpu *cpu, uint8_t device) {
}

static const struct i8080_io cpm_io = {
	.input = cpm_input, .output = cpm_output,
};

static int
i8080_cpu_init_from_file(struct i8080_cpu *cpu, const char *filename, uint16_t address) {
	const int fd = open(filename, O_RDONLY);

	if(fd == -1) {
		return errno;
	}

	i8080_cpu_init(cpu, &cpm_io);

	cpu->memory[0x0005] = 0xC9;
	cpu->memory[0x0006] = 0xFF;
	cpu->memory[0x0007] = 0xFF;

	cpu->pc = address;

	const ssize_t readval = read(fd, cpu->memory + address, I8080_MEMORY_SIZE - address);

	close(fd);

	if(readval == -1) {
		return errno;
	}

	return 0;
}

static int
i8080_cpu_print(const struct i8080_cpu *cpu, FILE *filep) {

	fprintf(filep,
		"i8080 cpu state:\n"
		"\t- uptime: %lu cycles\n"
		"\t- registers [b: 0x%.2X, c: 0x%.2X, d: 0x%.2X, e: 0x%.2X, h: 0x%.2X, l: 0x%.2X, a: 0x%.2X]\n"
		"\t- registers pairs [b: 0x%.4X, d: 0x%.4X, h: 0x%.4X, psw: 0x%.4X]\n"
		"\t- conditions flags [0x%.2X]\n"
		"\t- execution [sp: 0x%.4X, pc: 0x%.4X, inte: %d, stopped: %d]\n",
		cpu->uptime_cycles,
		cpu->registers.b, cpu->registers.c, cpu->registers.d, cpu->registers.e,
		cpu->registers.h, cpu->registers.l, cpu->registers.a,
		cpu->registers.pair.b, cpu->registers.pair.d, cpu->registers.pair.h, cpu->registers.pair.psw,
		cpu->registers.f,
		cpu->sp, cpu->pc, cpu->inte, cpu->stopped);

	return 0;
}

static void
i8080_usage(const char *i8080name) {
	fprintf(stderr, "usage: %s program\n", i8080name);
	exit(EXIT_FAILURE);
}

static struct i8080_args
i8080_parse_args(int argc, char **argv) {
	struct i8080_args args = {
	};
	int c;

	while((c = getopt(argc, argv, ":")) != -1) {
		switch(c) {
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

		while(!cpu.stopped) {
			switch(cpu.pc) {
			case 0:
				cpu.stopped = 1;
				break;
			case 5:
				if(cpu.registers.c == 2) {
					fputc(cpu.registers.e, stdout);
				} else if(cpu.registers.c == 9) {
					uint16_t address = cpu.registers.pair.d;
					uint8_t byte;

					while(byte = cpu.memory[address], byte != 0x24) {
						fputc(byte, stdout);
						address++;
					}
				}
				break;
			}

			i8080_cpu_next(&cpu);
		}

		i8080_cpu_deinit(&cpu);

		argpos++;
	}

	return EXIT_SUCCESS;
}
