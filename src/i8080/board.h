#ifndef I8080_BOARD_H
#define I8080_BOARD_H

#include <stdbool.h>

#include "i8080/cpu.h"

struct i8080_board {
	const struct i8080_io *io;
	void (*setup)(struct i8080_cpu *, const char *);
	void (*teardown)(struct i8080_cpu *);
	bool (*isonline)(struct i8080_cpu *);
	void (*poll)(struct i8080_cpu *);
	void (*sync)(struct i8080_cpu *);
};

/* I8080_BOARD_H */
#endif
