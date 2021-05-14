#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "cpm.h"

/* Wizard trick to create an output device simulating some CP/M's BIOS calls
 * until a real CP/M is emulated */
static const uint8_t cpm_bios[] = {
	0x76, /* 0x00: HLT */
	0x00, 0x00, 0x00, 0x00,
	0xCF, /* 0x05: RST 1 (CALL 0x08)*/
	0xFF, 0xFF, /* 0x06: Available memory */
	0xD3, 0x00, /* 0x08: OUT 0x00 */
	0x33, /* 0x0A: INX SP, we want to return from the procedure which called 0x05, so sp += 2 */
	0x33, /* 0x0A: INX SP */
	0xC9, /* 0x0A: RET */
};

static void
cpm_input(struct i8080_cpu *cpu, uint8_t device) {
}

static void
cpm_output(struct i8080_cpu *cpu, uint8_t device) {
	if(device != 0) {
		return;
	}

	switch(cpu->registers.c) {
	case 2:
		fputc(cpu->registers.e, stdout);
		break;
	case 9: {
		const uint8_t * const begin = cpu->memory + cpu->registers.pair.d,
			* const end = cpu->memory + sizeof(cpu->memory);
		const uint8_t * const strend = memchr(begin, '$', end - begin);

		fwrite(begin, 1, (strend == NULL ? end : strend) - begin, stdout);

	}	break;
	}
}

static void
cpm_board_setup(struct i8080_cpu *cpu, const char *filename) {
	const int fd = open(filename, O_RDONLY);

	if(fd == -1) {
		err(EXIT_FAILURE, "open %s", filename);
	}

	memcpy(cpu->memory, cpm_bios, sizeof(cpm_bios));

	cpu->pc = 0x100;

	uint8_t *next = cpu->memory + cpu->pc;
	size_t left = sizeof(cpu->memory) - cpu->pc;
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

