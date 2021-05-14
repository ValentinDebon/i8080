#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "cpm.h"

static void
cpm_input(struct i8080_cpu *cpu, uint8_t device) {
}

static void
cpm_output(struct i8080_cpu *cpu, uint8_t device) {
}

static void
cpm_board_setup(struct i8080_cpu *cpu, const char *filename) {
	const int fd = open(filename, O_RDONLY);

	if(fd == -1) {
		err(EXIT_FAILURE, "open %s", filename);
	}

	cpu->memory[0x0005] = 0xC9;
	cpu->memory[0x0006] = 0xFF;
	cpu->memory[0x0007] = 0xFF;

	cpu->pc = 0x100;

	uint8_t *next = cpu->memory + cpu->pc;
	size_t left = I8080_MEMORY_SIZE - cpu->pc;
	ssize_t readval;

	while(left != 0 && (readval = read(fd, next, left)) > 0) {
		next += readval;
		left -= readval;
	}

	const int errcode = errno;

	close(fd);

	if(readval == -1) {
		errno = errcode;
		err(EXIT_FAILURE, "read %s", filename);
	}
}

static void
cpm_board_teardown(struct i8080_cpu *cpu) {
}

static bool
cpm_board_isonline(struct i8080_cpu *cpu) {
	return !cpu->stopped;
}

static void
cpm_board_poll(struct i8080_cpu *cpu) {
	switch(cpu->pc) {
	case 0:
		cpu->stopped = 1;
		break;
	case 5:
		if(cpu->registers.c == 2) {
			fputc(cpu->registers.e, stdout);
		} else if(cpu->registers.c == 9) {
			uint16_t address = cpu->registers.pair.d;
			uint8_t byte;

			while(byte = cpu->memory[address], byte != 0x24) {
				fputc(byte, stdout);
				address++;
			}
		}
		break;
	}
}

static void
cpm_board_sync(struct i8080_cpu *cpu) {
}

static const struct i8080_io cpm_io = {
	.input = cpm_input, .output = cpm_output,
};

const struct i8080_board cpm_board = {
	.io = &cpm_io,
	.setup = cpm_board_setup,
	.teardown = cpm_board_teardown,
	.isonline = cpm_board_isonline,
	.poll = cpm_board_poll,
	.sync = cpm_board_sync,
};

