#ifndef I8080_CPU_H
#define I8080_CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "i8080/endianness.h"

#define I8080_MEMORY_SIZE 0x10000

#define I8080_BIT_CONDITION_CARRY           0
#define I8080_BIT_CONDITION_UNUSED1         1
#define I8080_BIT_CONDITION_PARITY          2
#define I8080_BIT_CONDITION_UNUSED2         3
#define I8080_BIT_CONDITION_AUXILIARY_CARRY 4
#define I8080_BIT_CONDITION_UNUSED3         5
#define I8080_BIT_CONDITION_ZERO            6
#define I8080_BIT_CONDITION_SIGN            7

#define I8080_MASK_CONDITION_CARRY           (1 << I8080_BIT_CONDITION_CARRY)
#define I8080_MASK_CONDITION_UNUSED1         (1 << I8080_BIT_CONDITION_UNUSED1)
#define I8080_MASK_CONDITION_PARITY          (1 << I8080_BIT_CONDITION_PARITY)
#define I8080_MASK_CONDITION_UNUSED2         (1 << I8080_BIT_CONDITION_UNUSED2)
#define I8080_MASK_CONDITION_AUXILIARY_CARRY (1 << I8080_BIT_CONDITION_AUXILIARY_CARRY )
#define I8080_MASK_CONDITION_UNUSED3         (1 << I8080_BIT_CONDITION_UNUSED3)
#define I8080_MASK_CONDITION_ZERO            (1 << I8080_BIT_CONDITION_ZERO)
#define I8080_MASK_CONDITION_SIGN            (1 << I8080_BIT_CONDITION_SIGN)

struct i8080_cpu;

union i8080_imm {
	uint16_t a16;
	uint16_t d16;
	uint8_t d8;
};

struct i8080_io {
	void (*input)(struct i8080_cpu *, uint8_t);
	void (*output)(struct i8080_cpu *, uint8_t);
};

struct i8080_rom_section {
	uint16_t begin, end;
};

struct i8080_instruction {
	const char *mnemonic;
	bool (*execute)(struct i8080_cpu *, union i8080_imm);
	unsigned length, nojump, onjump;
};

struct i8080_cpu {
	unsigned stopped : 1;
	unsigned inte : 1;
	union {
		struct {
#ifdef I8080_TARGET_LITTLE_ENDIAN
			uint8_t c;
			uint8_t b;
			uint8_t e;
			uint8_t d;
			uint8_t l;
			uint8_t h;
			uint8_t f;
			uint8_t a;
#endif
#ifdef I8080_TARGET_BIG_ENDIAN
			uint8_t b;
			uint8_t c;
			uint8_t d;
			uint8_t e;
			uint8_t h;
			uint8_t l;
			uint8_t a;
			uint8_t f;
#endif
		};
		struct {
			uint16_t b;
			uint16_t d;
			uint16_t h;
			uint16_t psw;
		} pair;
	} registers;
	uint16_t pc, sp;
	uint64_t uptime_cycles;
	const struct i8080_io *io;
	const struct i8080_rom_section *rom_map;
	uint8_t memory[I8080_MEMORY_SIZE];
};

int
i8080_cpu_init(struct i8080_cpu *cpu, const struct i8080_io *io);

int
i8080_cpu_deinit(struct i8080_cpu *cpu);

int
i8080_cpu_next(struct i8080_cpu *cpu);

int
i8080_cpu_interrupt(struct i8080_cpu *cpu, uint8_t opcode, union i8080_imm imm);

const struct i8080_instruction *
i8080_instruction_info(uint8_t opcode);

#ifdef I8080_TARGET_LITTLE_ENDIAN
_Static_assert(offsetof(struct i8080_cpu, registers.pair.b) + 1 == offsetof(struct i8080_cpu, registers.b), "Registers b is not aligned with register pair b");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.b) == offsetof(struct i8080_cpu, registers.c), "Registers c is not aligned with register pair b");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.d) + 1 == offsetof(struct i8080_cpu, registers.d), "Registers d is not aligned with register pair d");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.d) == offsetof(struct i8080_cpu, registers.e), "Registers e is not aligned with register pair d");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.h) + 1 == offsetof(struct i8080_cpu, registers.h), "Registers h is not aligned with register pair h");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.h) == offsetof(struct i8080_cpu, registers.l), "Registers l is not aligned with register pair h");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.psw) + 1 == offsetof(struct i8080_cpu, registers.a), "Registers a is not aligned with register pair psw");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.psw) == offsetof(struct i8080_cpu, registers.f), "Registers f is not aligned with register pair psw");
#endif

#ifdef I8080_TARGET_BIG_ENDIAN
_Static_assert(offsetof(struct i8080_cpu, registers.pair.b) == offsetof(struct i8080_cpu, registers.b), "Registers b is not aligned with register pair b");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.b) + 1 == offsetof(struct i8080_cpu, registers.c), "Registers c is not aligned with register pair b");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.d) == offsetof(struct i8080_cpu, registers.d), "Registers d is not aligned with register pair d");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.d) + 1 == offsetof(struct i8080_cpu, registers.e), "Registers e is not aligned with register pair d");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.h) == offsetof(struct i8080_cpu, registers.h), "Registers h is not aligned with register pair h");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.h) + 1 == offsetof(struct i8080_cpu, registers.l), "Registers l is not aligned with register pair h");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.psw) == offsetof(struct i8080_cpu, registers.a), "Registers a is not aligned with register pair psw");
_Static_assert(offsetof(struct i8080_cpu, registers.pair.psw) + 1 == offsetof(struct i8080_cpu, registers.f), "Registers f is not aligned with register pair psw");
#endif

/* I8080_CPU_H */
#endif
