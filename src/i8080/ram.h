#ifndef I8080_RAM_H
#define I8080_RAM_H

#include "i8080/cpu.h"

void
i8080_ram_load_file(struct i8080_cpu *cpu, const char *filename, uint16_t address);

/* I8080_RAM_H */
#endif
