#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "ram.h"

void
i8080_ram_load_file(struct i8080_cpu *cpu, const char *filename, uint16_t address) {
	const int fd = open(filename, O_RDONLY);

	if(fd == -1) {
		err(EXIT_FAILURE, "open %s", filename);
	}

	uint8_t *next = cpu->memory + address;
	size_t left = sizeof(cpu->memory) - address;
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
