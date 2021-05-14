#include <string.h>

#include "i8080/cpu.h"

/* The following macro detects if a carry was emitted at bit during the addition of lhs and rhs which lead to res */
#define I8080_CARRY_OUT(lhs, rhs, res, bit) ((~(res) & ((lhs) | (rhs)) | (lhs) & (rhs)) >> (bit) & 1)

/* The following macros generate the condition bit if the condition is met for the given operand */
#define I8080_CONDITION_CARRY(lhs, rhs, res)           (I8080_CARRY_OUT(lhs, rhs, res, sizeof(res) * 8 - 1) << I8080_BIT_CONDITION_CARRY)
#define I8080_CONDITION_PARITY(res)                    (!__builtin_parity(res) << I8080_BIT_CONDITION_PARITY)
#define I8080_CONDITION_AUXILIARY_CARRY(lhs, rhs, res) (I8080_CARRY_OUT(lhs, rhs, res, 3) << I8080_BIT_CONDITION_AUXILIARY_CARRY)
#define I8080_CONDITION_ZERO(res)                      (!(res) << I8080_BIT_CONDITION_ZERO)
#define I8080_CONDITION_SIGN(res)                      (((res) >> (sizeof(res) * 8 - 1)) << I8080_BIT_CONDITION_SIGN)

#define I8080_CONDITION_AUXILIARY_BORROW(lhs, rhs, res) I8080_CONDITION_AUXILIARY_CARRY(lhs, ~(rhs), res)
#define I8080_CONDITION_BORROW(lhs, rhs, res) (I8080_CONDITION_CARRY(lhs, ~(rhs), res) ^ I8080_MASK_CONDITION_CARRY)

/* The following masks help erase condition flags in several instruction (eg. INR, DCR...) */
#define I8080_MASK_CONDITIONS_SZ_A_P__ (I8080_MASK_CONDITION_SIGN |\
                                        I8080_MASK_CONDITION_ZERO |\
                                        I8080_MASK_CONDITION_AUXILIARY_CARRY |\
                                        I8080_MASK_CONDITION_PARITY)

#define I8080_MASK_CONDITIONS_SZ_A_P_C (I8080_MASK_CONDITIONS_SZ_A_P__ | I8080_MASK_CONDITION_CARRY)

/*****************
 * Memory access *
 *****************/

static inline void
i8080_cpu_store8(struct i8080_cpu *cpu, uint16_t address, uint8_t src) {
	cpu->memory[address] = src;
}

static inline void
i8080_cpu_store16(struct i8080_cpu *cpu, uint16_t address, uint16_t src) {
	i8080_cpu_store8(cpu, address, src);
	i8080_cpu_store8(cpu, address + 1, src >> 8);
}

static inline void
i8080_cpu_load8(const struct i8080_cpu *cpu, uint16_t address, uint8_t *dst) {
	*dst = cpu->memory[address];
}

static inline void
i8080_cpu_load16(const struct i8080_cpu *cpu, uint16_t address, uint16_t *dst) {
	*dst = (uint16_t)cpu->memory[address + 1] << 8 | cpu->memory[address];
}

/*********************************
 * Instructions and opcode table *
 *********************************/

/* NOP */

static bool
i8080_cpu_instruction_nop(struct i8080_cpu *cpu, union i8080_imm imm) {
	return false;
}

/* LXI */

static bool
i8080_cpu_instruction_lxi_b_d16(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.b = imm.d16;

	return false;
}

static bool
i8080_cpu_instruction_lxi_d_d16(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.d = imm.d16;

	return false;
}

static bool
i8080_cpu_instruction_lxi_h_d16(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.h = imm.d16;

	return false;
}

static bool
i8080_cpu_instruction_lxi_sp_d16(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->sp = imm.d16;

	return false;
}

/* STAX */

static bool
i8080_cpu_instruction_stax_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.b, cpu->registers.a);

	return false;
}

static bool
i8080_cpu_instruction_stax_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.d, cpu->registers.a);

	return false;
}

/* INX */

static bool
i8080_cpu_instruction_inx_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.b++;

	return false;
}

static bool
i8080_cpu_instruction_inx_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.d++;

	return false;
}

static bool
i8080_cpu_instruction_inx_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.h++;

	return false;
}

static bool
i8080_cpu_instruction_inx_sp(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->sp++;

	return false;
}

/* INR */

static inline bool
i8080_cpu_instruction_inr(struct i8080_cpu *cpu, uint8_t *dst) {
	const uint8_t res = *dst + 1;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P__
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_CARRY(*dst, 1, res)
		| I8080_CONDITION_PARITY(res);
	*dst = res;

	return false;
}

static bool
i8080_cpu_instruction_inr_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_inr(cpu, &cpu->registers.b);
}

static bool
i8080_cpu_instruction_inr_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_inr(cpu, &cpu->registers.c);
}

static bool
i8080_cpu_instruction_inr_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_inr(cpu, &cpu->registers.d);
}

static bool
i8080_cpu_instruction_inr_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_inr(cpu, &cpu->registers.e);
}

static bool
i8080_cpu_instruction_inr_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_inr(cpu, &cpu->registers.h);
}

static bool
i8080_cpu_instruction_inr_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_inr(cpu, &cpu->registers.l);
}

static bool
i8080_cpu_instruction_inr_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	bool jumped;
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);
	jumped = i8080_cpu_instruction_inr(cpu, &m);
	i8080_cpu_store8(cpu, cpu->registers.pair.h, m);

	return jumped;
}

static bool
i8080_cpu_instruction_inr_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_inr(cpu, &cpu->registers.a);
}

/* DCR */

static inline bool
i8080_cpu_instruction_dcr(struct i8080_cpu *cpu, uint8_t *dst) {
	const uint8_t res = *dst - 1;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P__
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_BORROW(*dst, 1, res)
		| I8080_CONDITION_PARITY(res);
	*dst = res;

	return false;
}

static bool
i8080_cpu_instruction_dcr_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dcr(cpu, &cpu->registers.b);
}

static bool
i8080_cpu_instruction_dcr_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dcr(cpu, &cpu->registers.c);
}

static bool
i8080_cpu_instruction_dcr_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dcr(cpu, &cpu->registers.d);
}

static bool
i8080_cpu_instruction_dcr_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dcr(cpu, &cpu->registers.e);
}

static bool
i8080_cpu_instruction_dcr_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dcr(cpu, &cpu->registers.h);
}

static bool
i8080_cpu_instruction_dcr_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dcr(cpu, &cpu->registers.l);
}

static bool
i8080_cpu_instruction_dcr_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	bool jumped;
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);
	jumped = i8080_cpu_instruction_dcr(cpu, &m);
	i8080_cpu_store8(cpu, cpu->registers.pair.h, m);

	return jumped;
}

static bool
i8080_cpu_instruction_dcr_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dcr(cpu, &cpu->registers.a);
}

/* MVI */

static bool
i8080_cpu_instruction_mvi_b_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.b = imm.d8;

	return false;
}

static bool
i8080_cpu_instruction_mvi_c_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.c = imm.d8;

	return false;
}

static bool
i8080_cpu_instruction_mvi_d_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.d = imm.d8;

	return false;
}

static bool
i8080_cpu_instruction_mvi_e_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.e = imm.d8;

	return false;
}

static bool
i8080_cpu_instruction_mvi_h_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.h = imm.d8;

	return false;
}

static bool
i8080_cpu_instruction_mvi_l_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.l = imm.d8;

	return false;
}

static bool
i8080_cpu_instruction_mvi_m_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.h, imm.d8);

	return false;
}

static bool
i8080_cpu_instruction_mvi_a_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = imm.d8;

	return false;
}

/* RLC */

static bool
i8080_cpu_instruction_rlc(struct i8080_cpu *cpu, union i8080_imm imm) {
	const uint8_t carry = cpu->registers.a >> 7;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITION_CARRY
		| carry << I8080_BIT_CONDITION_CARRY;
	cpu->registers.a = cpu->registers.a << 1 | carry;

	return false;
}

/* DAD */

static inline bool
i8080_cpu_instruction_dad(struct i8080_cpu *cpu, uint16_t src) {
	const uint16_t sum = cpu->registers.pair.h + src;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITION_CARRY
		| I8080_CONDITION_CARRY(cpu->registers.pair.h, src, sum);
	cpu->registers.pair.h = sum;

	return false;
}

static bool
i8080_cpu_instruction_dad_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dad(cpu, cpu->registers.pair.b);
}

static bool
i8080_cpu_instruction_dad_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dad(cpu, cpu->registers.pair.d);
}

static bool
i8080_cpu_instruction_dad_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dad(cpu, cpu->registers.pair.h);
}

static bool
i8080_cpu_instruction_dad_sp(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_dad(cpu, cpu->sp);
}

/* LDAX */

static bool
i8080_cpu_instruction_ldax_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.b, &cpu->registers.a);

	return false;
}

static bool
i8080_cpu_instruction_ldax_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.d, &cpu->registers.a);

	return false;
}

/* DCX */

static bool
i8080_cpu_instruction_dcx_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.b--;

	return false;
}

static bool
i8080_cpu_instruction_dcx_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.d--;

	return false;
}

static bool
i8080_cpu_instruction_dcx_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.pair.h--;

	return false;
}

static bool
i8080_cpu_instruction_dcx_sp(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->sp--;

	return false;
}

/* RRC */

static bool
i8080_cpu_instruction_rrc(struct i8080_cpu *cpu, union i8080_imm imm) {
	const uint8_t carry = cpu->registers.a & 1;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITION_CARRY
		| carry << I8080_BIT_CONDITION_CARRY;
	cpu->registers.a = cpu->registers.a >> 1 | carry << 7;

	return false;
}

/* RAL */

static bool
i8080_cpu_instruction_ral(struct i8080_cpu *cpu, union i8080_imm imm) {
	const uint8_t lsbit = (cpu->registers.f & I8080_MASK_CONDITION_CARRY) >> I8080_BIT_CONDITION_CARRY;
	const uint8_t carry = cpu->registers.a >> 7;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITION_CARRY
		| carry << I8080_BIT_CONDITION_CARRY;
	cpu->registers.a = cpu->registers.a << 1 | lsbit;

	return false;
}

/* RAR */

static bool
i8080_cpu_instruction_rar(struct i8080_cpu *cpu, union i8080_imm imm) {
	const uint8_t msbit = (cpu->registers.f & I8080_MASK_CONDITION_CARRY) << (7 - I8080_BIT_CONDITION_CARRY);
	const uint8_t carry = cpu->registers.a & 1;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITION_CARRY
		| carry << I8080_BIT_CONDITION_CARRY;
	cpu->registers.a = cpu->registers.a >> 1 | msbit;

	return false;
}

/* SHLD */

static bool
i8080_cpu_instruction_shld_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store16(cpu, imm.a16, cpu->registers.pair.h);

	return false;
}

/* DAA */

static bool
i8080_cpu_instruction_daa(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t low = cpu->registers.a & 0x0F, src = 0, carry = 0;

	if(low > 9
		|| (cpu->registers.f & I8080_MASK_CONDITION_AUXILIARY_CARRY) != 0) {
		src |= 0x06;
		low += src;
	}

	if(((cpu->registers.a >> 4) + (low >> 4)) > 9
		|| (cpu->registers.f & I8080_MASK_CONDITION_CARRY) != 0) {
		src |= 0x60;
		carry = I8080_MASK_CONDITION_CARRY;
	}

	const uint8_t res = cpu->registers.a + src;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_CARRY(cpu->registers.a, src, res)
		| I8080_CONDITION_PARITY(res)
		| carry;
	cpu->registers.a = res;

	return false;
}

/* LHLD */

static bool
i8080_cpu_instruction_lhld_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load16(cpu, imm.a16, &cpu->registers.pair.h);

	return false;
}

/* CMA */

static bool
i8080_cpu_instruction_cma(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = ~cpu->registers.a;

	return false;
}

/* STA */

static bool
i8080_cpu_instruction_sta_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, imm.a16, cpu->registers.a);

	return false;
}

/* STC */

static bool
i8080_cpu_instruction_stc(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.f |= I8080_MASK_CONDITION_CARRY;

	return false;
}

/* LDA */

static bool
i8080_cpu_instruction_lda_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, imm.a16, &cpu->registers.a);

	return false;
}

/* CMC */

static bool
i8080_cpu_instruction_cmc(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.f ^= I8080_MASK_CONDITION_CARRY;

	return false;
}

/* MOV */

static bool
i8080_cpu_instruction_mov_b_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.b = cpu->registers.b;

	return false;
}

static bool
i8080_cpu_instruction_mov_b_c(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.b = cpu->registers.c;

	return false;
}

static bool
i8080_cpu_instruction_mov_b_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.b = cpu->registers.d;

	return false;
}

static bool
i8080_cpu_instruction_mov_b_e(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.b = cpu->registers.e;

	return false;
}

static bool
i8080_cpu_instruction_mov_b_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.b = cpu->registers.h;

	return false;
}

static bool
i8080_cpu_instruction_mov_b_l(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.b = cpu->registers.l;

	return false;
}

static bool
i8080_cpu_instruction_mov_b_m(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &cpu->registers.b);

	return false;
}

static bool
i8080_cpu_instruction_mov_b_a(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.b = cpu->registers.a;

	return false;
}

static bool
i8080_cpu_instruction_mov_c_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.c = cpu->registers.b;

	return false;
}

static bool
i8080_cpu_instruction_mov_c_c(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.c = cpu->registers.c;

	return false;
}

static bool
i8080_cpu_instruction_mov_c_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.c = cpu->registers.d;

	return false;
}

static bool
i8080_cpu_instruction_mov_c_e(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.c = cpu->registers.e;

	return false;
}

static bool
i8080_cpu_instruction_mov_c_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.c = cpu->registers.h;

	return false;
}

static bool
i8080_cpu_instruction_mov_c_l(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.c = cpu->registers.l;

	return false;
}

static bool
i8080_cpu_instruction_mov_c_m(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &cpu->registers.c);

	return false;
}

static bool
i8080_cpu_instruction_mov_c_a(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.c = cpu->registers.a;

	return false;
}

static bool
i8080_cpu_instruction_mov_d_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.d = cpu->registers.b;

	return false;
}

static bool
i8080_cpu_instruction_mov_d_c(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.d = cpu->registers.c;

	return false;
}

static bool
i8080_cpu_instruction_mov_d_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.d = cpu->registers.d;

	return false;
}

static bool
i8080_cpu_instruction_mov_d_e(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.d = cpu->registers.e;

	return false;
}

static bool
i8080_cpu_instruction_mov_d_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.d = cpu->registers.h;

	return false;
}

static bool
i8080_cpu_instruction_mov_d_l(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.d = cpu->registers.l;

	return false;
}

static bool
i8080_cpu_instruction_mov_d_m(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &cpu->registers.d);

	return false;
}

static bool
i8080_cpu_instruction_mov_d_a(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.d = cpu->registers.a;

	return false;
}

static bool
i8080_cpu_instruction_mov_e_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.e = cpu->registers.b;

	return false;
}

static bool
i8080_cpu_instruction_mov_e_c(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.e = cpu->registers.c;

	return false;
}

static bool
i8080_cpu_instruction_mov_e_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.e = cpu->registers.d;

	return false;
}

static bool
i8080_cpu_instruction_mov_e_e(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.e = cpu->registers.e;

	return false;
}

static bool
i8080_cpu_instruction_mov_e_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.e = cpu->registers.h;

	return false;
}

static bool
i8080_cpu_instruction_mov_e_l(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.e = cpu->registers.l;

	return false;
}

static bool
i8080_cpu_instruction_mov_e_m(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &cpu->registers.e);

	return false;
}

static bool
i8080_cpu_instruction_mov_e_a(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.e = cpu->registers.a;

	return false;
}

static bool
i8080_cpu_instruction_mov_h_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.h = cpu->registers.b;

	return false;
}

static bool
i8080_cpu_instruction_mov_h_c(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.h = cpu->registers.c;

	return false;
}

static bool
i8080_cpu_instruction_mov_h_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.h = cpu->registers.d;

	return false;
}

static bool
i8080_cpu_instruction_mov_h_e(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.h = cpu->registers.e;

	return false;
}

static bool
i8080_cpu_instruction_mov_h_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.h = cpu->registers.h;

	return false;
}

static bool
i8080_cpu_instruction_mov_h_l(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.h = cpu->registers.l;

	return false;
}

static bool
i8080_cpu_instruction_mov_h_m(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &cpu->registers.h);

	return false;
}

static bool
i8080_cpu_instruction_mov_h_a(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.h = cpu->registers.a;

	return false;
}

static bool
i8080_cpu_instruction_mov_l_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.l = cpu->registers.b;

	return false;
}

static bool
i8080_cpu_instruction_mov_l_c(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.l = cpu->registers.c;

	return false;
}

static bool
i8080_cpu_instruction_mov_l_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.l = cpu->registers.d;

	return false;
}

static bool
i8080_cpu_instruction_mov_l_e(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.l = cpu->registers.e;

	return false;
}

static bool
i8080_cpu_instruction_mov_l_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.l = cpu->registers.h;

	return false;
}

static bool
i8080_cpu_instruction_mov_l_l(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.l = cpu->registers.l;

	return false;
}

static bool
i8080_cpu_instruction_mov_l_m(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &cpu->registers.l);

	return false;
}

static bool
i8080_cpu_instruction_mov_l_a(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.l = cpu->registers.a;

	return false;
}

static bool
i8080_cpu_instruction_mov_m_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.h, cpu->registers.b);

	return false;
}

static bool
i8080_cpu_instruction_mov_m_c(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.h, cpu->registers.c);

	return false;
}

static bool
i8080_cpu_instruction_mov_m_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.h, cpu->registers.d);

	return false;
}

static bool
i8080_cpu_instruction_mov_m_e(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.h, cpu->registers.e);

	return false;
}

static bool
i8080_cpu_instruction_mov_m_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.h, cpu->registers.h);

	return false;
}

static bool
i8080_cpu_instruction_mov_m_l(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.h, cpu->registers.l);

	return false;
}

static bool
i8080_cpu_instruction_mov_m_a(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_store8(cpu, cpu->registers.pair.h, cpu->registers.a);

	return false;
}

static bool
i8080_cpu_instruction_mov_a_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = cpu->registers.b;

	return false;
}

static bool
i8080_cpu_instruction_mov_a_c(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = cpu->registers.c;

	return false;
}

static bool
i8080_cpu_instruction_mov_a_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = cpu->registers.d;

	return false;
}

static bool
i8080_cpu_instruction_mov_a_e(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = cpu->registers.e;

	return false;
}

static bool
i8080_cpu_instruction_mov_a_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = cpu->registers.h;

	return false;
}

static bool
i8080_cpu_instruction_mov_a_l(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = cpu->registers.l;

	return false;
}

static bool
i8080_cpu_instruction_mov_a_m(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &cpu->registers.a);

	return false;
}

static bool
i8080_cpu_instruction_mov_a_a(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->registers.a = cpu->registers.a;

	return false;
}

/* HLT */

static bool
i8080_cpu_instruction_hlt(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->stopped = 1;

	return false;
}

/* ADD */

static inline bool
i8080_cpu_instruction_add(struct i8080_cpu *cpu, uint8_t src) {
	const uint8_t res = cpu->registers.a + src;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_CARRY(cpu->registers.a, src, res)
		| I8080_CONDITION_PARITY(res)
		| I8080_CONDITION_CARRY(cpu->registers.a, src, res);
	cpu->registers.a = res;

	return false;
}

static bool
i8080_cpu_instruction_add_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_add(cpu, cpu->registers.b);
}

static bool
i8080_cpu_instruction_add_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_add(cpu, cpu->registers.c);
}

static bool
i8080_cpu_instruction_add_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_add(cpu, cpu->registers.d);
}

static bool
i8080_cpu_instruction_add_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_add(cpu, cpu->registers.e);
}

static bool
i8080_cpu_instruction_add_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_add(cpu, cpu->registers.h);
}

static bool
i8080_cpu_instruction_add_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_add(cpu, cpu->registers.l);
}

static bool
i8080_cpu_instruction_add_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);

	return i8080_cpu_instruction_add(cpu, m);
}

static bool
i8080_cpu_instruction_add_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_add(cpu, cpu->registers.a);
}

/* ADI */

static bool
i8080_cpu_instruction_adi_d8(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_add(cpu, imm.d8);
}

/* ADC */

static inline bool
i8080_cpu_instruction_adc(struct i8080_cpu *cpu, uint8_t src) {
	const uint8_t carry = !!(cpu->registers.f & I8080_MASK_CONDITION_CARRY);
	const uint8_t propagated = src + carry;
	const uint8_t res = cpu->registers.a + propagated;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_CARRY(cpu->registers.a, propagated, res)
			^ I8080_CONDITION_AUXILIARY_CARRY(src, carry, propagated)
		| I8080_CONDITION_PARITY(res)
		| I8080_CONDITION_CARRY(cpu->registers.a, propagated, res)
			^ I8080_CONDITION_CARRY(src, carry, propagated);
	cpu->registers.a = res;

	return false;
}

static bool
i8080_cpu_instruction_adc_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_adc(cpu, cpu->registers.b);
}

static bool
i8080_cpu_instruction_adc_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_adc(cpu, cpu->registers.c);
}

static bool
i8080_cpu_instruction_adc_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_adc(cpu, cpu->registers.d);
}

static bool
i8080_cpu_instruction_adc_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_adc(cpu, cpu->registers.e);
}

static bool
i8080_cpu_instruction_adc_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_adc(cpu, cpu->registers.h);
}

static bool
i8080_cpu_instruction_adc_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_adc(cpu, cpu->registers.l);
}

static bool
i8080_cpu_instruction_adc_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);

	return i8080_cpu_instruction_adc(cpu, m);
}

static bool
i8080_cpu_instruction_adc_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_adc(cpu, cpu->registers.a);
}

/* ACI */

static bool
i8080_cpu_instruction_aci_d8(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_adc(cpu, imm.d8);
}

/* SUB */

static inline bool
i8080_cpu_instruction_sub(struct i8080_cpu *cpu, uint8_t src) {
	const uint8_t res = cpu->registers.a - src;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_BORROW(cpu->registers.a, src, res)
		| I8080_CONDITION_PARITY(res)
		| I8080_CONDITION_BORROW(cpu->registers.a, src, res);
	cpu->registers.a = res;

	return false;
}

static bool
i8080_cpu_instruction_sub_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sub(cpu, cpu->registers.b);
}

static bool
i8080_cpu_instruction_sub_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sub(cpu, cpu->registers.c);
}

static bool
i8080_cpu_instruction_sub_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sub(cpu, cpu->registers.d);
}

static bool
i8080_cpu_instruction_sub_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sub(cpu, cpu->registers.e);
}

static bool
i8080_cpu_instruction_sub_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sub(cpu, cpu->registers.h);
}

static bool
i8080_cpu_instruction_sub_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sub(cpu, cpu->registers.l);
}

static bool
i8080_cpu_instruction_sub_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);

	return i8080_cpu_instruction_sub(cpu, m);
}

static bool
i8080_cpu_instruction_sub_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sub(cpu, cpu->registers.a);
}

/* SUI */

static bool
i8080_cpu_instruction_sui_d8(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sub(cpu, imm.d8);
}

/* SBB */

static inline bool
i8080_cpu_instruction_sbb(struct i8080_cpu *cpu, uint8_t src) {
	const uint8_t carry = !!(cpu->registers.f & I8080_MASK_CONDITION_CARRY);
	const uint8_t propagated = src + carry;
	const uint8_t res = cpu->registers.a - propagated;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_BORROW(cpu->registers.a, propagated, res)
			^ I8080_CONDITION_AUXILIARY_CARRY(src, carry, propagated)
		| I8080_CONDITION_PARITY(res)
		| I8080_CONDITION_BORROW(cpu->registers.a, propagated, res)
			^ I8080_CONDITION_CARRY(src, carry, propagated);
	cpu->registers.a = res;

	return false;
}

static bool
i8080_cpu_instruction_sbb_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sbb(cpu, cpu->registers.b);
}

static bool
i8080_cpu_instruction_sbb_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sbb(cpu, cpu->registers.c);
}

static bool
i8080_cpu_instruction_sbb_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sbb(cpu, cpu->registers.d);
}

static bool
i8080_cpu_instruction_sbb_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sbb(cpu, cpu->registers.e);
}

static bool
i8080_cpu_instruction_sbb_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sbb(cpu, cpu->registers.h);
}

static bool
i8080_cpu_instruction_sbb_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sbb(cpu, cpu->registers.l);
}

static bool
i8080_cpu_instruction_sbb_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);

	return i8080_cpu_instruction_sbb(cpu, m);
}

static bool
i8080_cpu_instruction_sbb_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sbb(cpu, cpu->registers.a);
}

/* SBI */

static bool
i8080_cpu_instruction_sbi_d8(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_sbb(cpu, imm.d8);
}

/* ANA */

static inline bool
i8080_cpu_instruction_ana(struct i8080_cpu *cpu, uint8_t src) {
	const uint8_t res = cpu->registers.a & src;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_CARRY(cpu->registers.a, src, res)
		| I8080_CONDITION_PARITY(res);
	cpu->registers.a = res;

	return false;
}

static bool
i8080_cpu_instruction_ana_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ana(cpu, cpu->registers.b);
}

static bool
i8080_cpu_instruction_ana_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ana(cpu, cpu->registers.c);
}

static bool
i8080_cpu_instruction_ana_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ana(cpu, cpu->registers.d);
}

static bool
i8080_cpu_instruction_ana_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ana(cpu, cpu->registers.e);
}

static bool
i8080_cpu_instruction_ana_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ana(cpu, cpu->registers.h);
}

static bool
i8080_cpu_instruction_ana_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ana(cpu, cpu->registers.l);
}

static bool
i8080_cpu_instruction_ana_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);

	return i8080_cpu_instruction_ana(cpu, m);
}

static bool
i8080_cpu_instruction_ana_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ana(cpu, cpu->registers.a);
}

/* ANI */

static bool
i8080_cpu_instruction_ani_d8(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ana(cpu, imm.d8);
}

/* XRA */

static inline bool
i8080_cpu_instruction_xra(struct i8080_cpu *cpu, uint8_t src) {
	const uint8_t res = cpu->registers.a ^ src;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_PARITY(res);
	cpu->registers.a = res;

	return false;
}

static bool
i8080_cpu_instruction_xra_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_xra(cpu, cpu->registers.b);
}

static bool
i8080_cpu_instruction_xra_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_xra(cpu, cpu->registers.c);
}

static bool
i8080_cpu_instruction_xra_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_xra(cpu, cpu->registers.d);
}

static bool
i8080_cpu_instruction_xra_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_xra(cpu, cpu->registers.e);
}

static bool
i8080_cpu_instruction_xra_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_xra(cpu, cpu->registers.h);
}

static bool
i8080_cpu_instruction_xra_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_xra(cpu, cpu->registers.l);
}

static bool
i8080_cpu_instruction_xra_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);

	return i8080_cpu_instruction_xra(cpu, m);
}

static bool
i8080_cpu_instruction_xra_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_xra(cpu, cpu->registers.a);
}

/* XRI */

static bool
i8080_cpu_instruction_xri_d8(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_xra(cpu, imm.d8);
}

/* ORA */

static inline bool
i8080_cpu_instruction_ora(struct i8080_cpu *cpu, uint8_t src) {
	const uint8_t res = cpu->registers.a | src;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_PARITY(res);
	cpu->registers.a = res;

	return false;
}

static bool
i8080_cpu_instruction_ora_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ora(cpu, cpu->registers.b);
}

static bool
i8080_cpu_instruction_ora_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ora(cpu, cpu->registers.c);
}

static bool
i8080_cpu_instruction_ora_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ora(cpu, cpu->registers.d);
}

static bool
i8080_cpu_instruction_ora_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ora(cpu, cpu->registers.e);
}

static bool
i8080_cpu_instruction_ora_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ora(cpu, cpu->registers.h);
}

static bool
i8080_cpu_instruction_ora_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ora(cpu, cpu->registers.l);
}

static bool
i8080_cpu_instruction_ora_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);

	return i8080_cpu_instruction_ora(cpu, m);
}

static bool
i8080_cpu_instruction_ora_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ora(cpu, cpu->registers.a);
}

/* ORI */

static bool
i8080_cpu_instruction_ori_d8(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_ora(cpu, imm.d8);
}

/* CMP */

static inline bool
i8080_cpu_instruction_cmp(struct i8080_cpu *cpu, uint8_t src) {
	const uint8_t res = cpu->registers.a - src;

	cpu->registers.f = cpu->registers.f & ~I8080_MASK_CONDITIONS_SZ_A_P_C
		| I8080_CONDITION_SIGN(res)
		| I8080_CONDITION_ZERO(res)
		| I8080_CONDITION_AUXILIARY_BORROW(cpu->registers.a, src, res)
		| I8080_CONDITION_PARITY(res)
		| I8080_CONDITION_BORROW(cpu->registers.a, src, res);

	return false;
}

static bool
i8080_cpu_instruction_cmp_b(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_cmp(cpu, cpu->registers.b);
}

static bool
i8080_cpu_instruction_cmp_c(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_cmp(cpu, cpu->registers.c);
}

static bool
i8080_cpu_instruction_cmp_d(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_cmp(cpu, cpu->registers.d);
}

static bool
i8080_cpu_instruction_cmp_e(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_cmp(cpu, cpu->registers.e);
}

static bool
i8080_cpu_instruction_cmp_h(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_cmp(cpu, cpu->registers.h);
}

static bool
i8080_cpu_instruction_cmp_l(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_cmp(cpu, cpu->registers.l);
}

static bool
i8080_cpu_instruction_cmp_m(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint8_t m;

	i8080_cpu_load8(cpu, cpu->registers.pair.h, &m);

	return i8080_cpu_instruction_cmp(cpu, m);
}

static bool
i8080_cpu_instruction_cmp_a(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_cmp(cpu, cpu->registers.a);
}

/* CPI */

static bool
i8080_cpu_instruction_cpi_d8(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_cmp(cpu, imm.d8);
}

/* RET... */

static inline bool
i8080_cpu_instruction_ret(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load16(cpu, cpu->sp, &cpu->pc);
	cpu->sp += sizeof(uint16_t);

	return true;
}

static bool
i8080_cpu_instruction_rnz(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_ZERO)) {
		return i8080_cpu_instruction_ret(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_rz(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_ZERO) {
		return i8080_cpu_instruction_ret(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_rnc(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_CARRY)) {
		return i8080_cpu_instruction_ret(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_rc(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_CARRY) {
		return i8080_cpu_instruction_ret(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_rpo(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_PARITY)) {
		return i8080_cpu_instruction_ret(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_rpe(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_PARITY) {
		return i8080_cpu_instruction_ret(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_rp(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_SIGN)) {
		return i8080_cpu_instruction_ret(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_rm(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_SIGN) {
		return i8080_cpu_instruction_ret(cpu, imm);
	}

	return false;
}

/* POP */

static bool
i8080_cpu_instruction_pop_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load16(cpu, cpu->sp, &cpu->registers.pair.b);
	cpu->sp += sizeof(uint16_t);

	return false;
}

static bool
i8080_cpu_instruction_pop_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load16(cpu, cpu->sp, &cpu->registers.pair.d);
	cpu->sp += sizeof(uint16_t);

	return false;
}

static bool
i8080_cpu_instruction_pop_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load16(cpu, cpu->sp, &cpu->registers.pair.h);
	cpu->sp += sizeof(uint16_t);

	return false;
}

static bool
i8080_cpu_instruction_pop_psw(struct i8080_cpu *cpu, union i8080_imm imm) {

	i8080_cpu_load16(cpu, cpu->sp, &cpu->registers.pair.psw);
	cpu->registers.f = cpu->registers.f & I8080_MASK_CONDITIONS_SZ_A_P_C | I8080_MASK_CONDITION_UNUSED1;
	cpu->sp += sizeof(uint16_t);

	return false;
}

/* JMP... */

static inline bool
i8080_cpu_instruction_jmp_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->pc = imm.a16;

	return true;
}

static bool
i8080_cpu_instruction_jnz_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_ZERO)) {
		return i8080_cpu_instruction_jmp_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_jz_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_ZERO) {
		return i8080_cpu_instruction_jmp_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_jnc_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_CARRY)) {
		return i8080_cpu_instruction_jmp_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_jc_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_CARRY) {
		return i8080_cpu_instruction_jmp_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_jpo_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_PARITY)) {
		return i8080_cpu_instruction_jmp_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_jpe_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_PARITY) {
		return i8080_cpu_instruction_jmp_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_jp_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_SIGN)) {
		return i8080_cpu_instruction_jmp_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_jm_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_SIGN) {
		return i8080_cpu_instruction_jmp_a16(cpu, imm);
	}

	return false;
}

/* CALL... */

static inline bool
i8080_cpu_instruction_call(struct i8080_cpu *cpu, uint16_t address) {

	cpu->sp -= sizeof(uint16_t);
	i8080_cpu_store16(cpu, cpu->sp, cpu->pc);
	cpu->pc = address;

	return true;
}

static inline bool
i8080_cpu_instruction_call_a16(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, imm.a16);
}

static bool
i8080_cpu_instruction_cnz_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_ZERO)) {
		return i8080_cpu_instruction_call_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_cz_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_ZERO) {
		return i8080_cpu_instruction_call_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_cnc_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_CARRY)) {
		return i8080_cpu_instruction_call_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_cc_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_CARRY) {
		return i8080_cpu_instruction_call_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_cpo_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_PARITY)) {
		return i8080_cpu_instruction_call_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_cpe_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_PARITY) {
		return i8080_cpu_instruction_call_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_cp_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(!(cpu->registers.f & I8080_MASK_CONDITION_SIGN)) {
		return i8080_cpu_instruction_call_a16(cpu, imm);
	}

	return false;
}

static bool
i8080_cpu_instruction_cm_a16(struct i8080_cpu *cpu, union i8080_imm imm) {

	if(cpu->registers.f & I8080_MASK_CONDITION_SIGN) {
		return i8080_cpu_instruction_call_a16(cpu, imm);
	}

	return false;
}

/* PUSH */

static bool
i8080_cpu_instruction_push_b(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->sp -= sizeof(uint16_t);
	i8080_cpu_store16(cpu, cpu->sp, cpu->registers.pair.b);

	return false;
}

static bool
i8080_cpu_instruction_push_d(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->sp -= sizeof(uint16_t);
	i8080_cpu_store16(cpu, cpu->sp, cpu->registers.pair.d);

	return false;
}

static bool
i8080_cpu_instruction_push_h(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->sp -= sizeof(uint16_t);
	i8080_cpu_store16(cpu, cpu->sp, cpu->registers.pair.h);

	return false;
}

static bool
i8080_cpu_instruction_push_psw(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->sp -= sizeof(uint16_t);
	i8080_cpu_store16(cpu, cpu->sp, cpu->registers.pair.psw);

	return false;
}

/* RST */

static bool
i8080_cpu_instruction_rst_0(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, 0x00);
}

static bool
i8080_cpu_instruction_rst_1(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, 0x08);
}

static bool
i8080_cpu_instruction_rst_2(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, 0x10);
}

static bool
i8080_cpu_instruction_rst_3(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, 0x18);
}

static bool
i8080_cpu_instruction_rst_4(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, 0x20);
}

static bool
i8080_cpu_instruction_rst_5(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, 0x28);
}

static bool
i8080_cpu_instruction_rst_6(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, 0x30);
}

static bool
i8080_cpu_instruction_rst_7(struct i8080_cpu *cpu, union i8080_imm imm) {
	return i8080_cpu_instruction_call(cpu, 0x38);
}

/* OUT */

static bool
i8080_cpu_instruction_out_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->io->output(cpu, imm.d8);

	return false;
}

/* IN */

static bool
i8080_cpu_instruction_in_d8(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->io->input(cpu, imm.d8);

	return false;
}

/* XTHL */

static bool
i8080_cpu_instruction_xthl(struct i8080_cpu *cpu, union i8080_imm imm) {
	uint16_t swap;

	i8080_cpu_load16(cpu, cpu->sp, &swap);
	i8080_cpu_store16(cpu, cpu->sp, cpu->registers.pair.h);
	cpu->registers.pair.h = swap;

	return false;
}

/* PCHL */

static bool
i8080_cpu_instruction_pchl(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->pc = cpu->registers.pair.h;

	return true;
}

/* XCHG */

static bool
i8080_cpu_instruction_xchg(struct i8080_cpu *cpu, union i8080_imm imm) {
	const uint16_t swap = cpu->registers.pair.d;

	cpu->registers.pair.d = cpu->registers.pair.h;
	cpu->registers.pair.h = swap;

	return false;
}

/* DI */

static bool
i8080_cpu_instruction_di(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->inte = 0;

	return false;
}

/* SPHL */

static bool
i8080_cpu_instruction_sphl(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->sp = cpu->registers.pair.h;

	return false;
}

/* EI */

static bool
i8080_cpu_instruction_ei(struct i8080_cpu *cpu, union i8080_imm imm) {

	cpu->inte = 1;

	return false;
}

static const struct i8080_instruction instructions[] = {
	/* 0x00 */ { "NOP",        .execute = i8080_cpu_instruction_nop,        .length = 1, .nojump =  4 },
	/* 0x01 */ { "LXI B D16",  .execute = i8080_cpu_instruction_lxi_b_d16,  .length = 3, .nojump = 10 },
	/* 0x02 */ { "STAX B",     .execute = i8080_cpu_instruction_stax_b,     .length = 1, .nojump =  7 },
	/* 0x03 */ { "INX B",      .execute = i8080_cpu_instruction_inx_b,      .length = 1, .nojump =  5 },
	/* 0x04 */ { "INR B",      .execute = i8080_cpu_instruction_inr_b,      .length = 1, .nojump =  5 },
	/* 0x05 */ { "DCR B",      .execute = i8080_cpu_instruction_dcr_b,      .length = 1, .nojump =  5 },
	/* 0x06 */ { "MVI B D8",   .execute = i8080_cpu_instruction_mvi_b_d8,   .length = 2, .nojump =  7 },
	/* 0x07 */ { "RLC",        .execute = i8080_cpu_instruction_rlc,        .length = 1, .nojump =  4 },
	/* 0x08 */ { "NOP",        .execute = i8080_cpu_instruction_nop,        .length = 1, .nojump =  4 },
	/* 0x09 */ { "DAD B",      .execute = i8080_cpu_instruction_dad_b,      .length = 1, .nojump = 10 },
	/* 0x0A */ { "LDAX B",     .execute = i8080_cpu_instruction_ldax_b,     .length = 1, .nojump =  7 },
	/* 0x0B */ { "DCX B",      .execute = i8080_cpu_instruction_dcx_b,      .length = 1, .nojump =  5 },
	/* 0x0C */ { "INR C",      .execute = i8080_cpu_instruction_inr_c,      .length = 1, .nojump =  5 },
	/* 0x0D */ { "DCR C",      .execute = i8080_cpu_instruction_dcr_c,      .length = 1, .nojump =  5 },
	/* 0x0E */ { "MVI C D8",   .execute = i8080_cpu_instruction_mvi_c_d8,   .length = 2, .nojump =  7 },
	/* 0x0F */ { "RRC",        .execute = i8080_cpu_instruction_rrc,        .length = 1, .nojump =  4 },
	/* 0x10 */ { "NOP",        .execute = i8080_cpu_instruction_nop,        .length = 1, .nojump =  4 },
	/* 0x11 */ { "LXI D D16",  .execute = i8080_cpu_instruction_lxi_d_d16,  .length = 3, .nojump = 10 },
	/* 0x12 */ { "STAX D",     .execute = i8080_cpu_instruction_stax_d,     .length = 1, .nojump =  7 },
	/* 0x13 */ { "INX D",      .execute = i8080_cpu_instruction_inx_d,      .length = 1, .nojump =  5 },
	/* 0x14 */ { "INR D",      .execute = i8080_cpu_instruction_inr_d,      .length = 1, .nojump =  5 },
	/* 0x15 */ { "DCR D",      .execute = i8080_cpu_instruction_dcr_d,      .length = 1, .nojump =  5 },
	/* 0x16 */ { "MVI D D8",   .execute = i8080_cpu_instruction_mvi_d_d8,   .length = 2, .nojump =  7 },
	/* 0x17 */ { "RAL",        .execute = i8080_cpu_instruction_ral,        .length = 1, .nojump =  4 },
	/* 0x18 */ { "NOP",        .execute = i8080_cpu_instruction_nop,        .length = 1, .nojump =  4 },
	/* 0x19 */ { "DAD D",      .execute = i8080_cpu_instruction_dad_d,      .length = 1, .nojump = 10 },
	/* 0x1A */ { "LDAX D",     .execute = i8080_cpu_instruction_ldax_d,     .length = 1, .nojump =  7 },
	/* 0x1B */ { "DCX D",      .execute = i8080_cpu_instruction_dcx_d,      .length = 1, .nojump =  5 },
	/* 0x1C */ { "INR E",      .execute = i8080_cpu_instruction_inr_e,      .length = 1, .nojump =  5 },
	/* 0x1D */ { "DCR E",      .execute = i8080_cpu_instruction_dcr_e,      .length = 1, .nojump =  5 },
	/* 0x1E */ { "MVI E D8",   .execute = i8080_cpu_instruction_mvi_e_d8,   .length = 2, .nojump =  7 },
	/* 0x1F */ { "RAR",        .execute = i8080_cpu_instruction_rar,        .length = 1, .nojump =  4 },
	/* 0x20 */ { "NOP",        .execute = i8080_cpu_instruction_nop,        .length = 1, .nojump =  4 },
	/* 0x21 */ { "LXI H D16",  .execute = i8080_cpu_instruction_lxi_h_d16,  .length = 3, .nojump = 10 },
	/* 0x22 */ { "SHLD A16",   .execute = i8080_cpu_instruction_shld_a16,   .length = 3, .nojump = 16 },
	/* 0x23 */ { "INX H",      .execute = i8080_cpu_instruction_inx_h,      .length = 1, .nojump =  5 },
	/* 0x24 */ { "INR H",      .execute = i8080_cpu_instruction_inr_h,      .length = 1, .nojump =  5 },
	/* 0x25 */ { "DCR H",      .execute = i8080_cpu_instruction_dcr_h,      .length = 1, .nojump =  5 },
	/* 0x26 */ { "MVI H D8",   .execute = i8080_cpu_instruction_mvi_h_d8,   .length = 2, .nojump =  7 },
	/* 0x27 */ { "DAA",        .execute = i8080_cpu_instruction_daa,        .length = 1, .nojump =  4 },
	/* 0x28 */ { "NOP",        .execute = i8080_cpu_instruction_nop,        .length = 1, .nojump =  4 },
	/* 0x29 */ { "DAD H",      .execute = i8080_cpu_instruction_dad_h,      .length = 1, .nojump = 10 },
	/* 0x2A */ { "LHLD A16",   .execute = i8080_cpu_instruction_lhld_a16,   .length = 3, .nojump = 16 },
	/* 0x2B */ { "DCX H",      .execute = i8080_cpu_instruction_dcx_h,      .length = 1, .nojump =  5 },
	/* 0x2C */ { "INR L",      .execute = i8080_cpu_instruction_inr_l,      .length = 1, .nojump =  5 },
	/* 0x2D */ { "DCR L",      .execute = i8080_cpu_instruction_dcr_l,      .length = 1, .nojump =  5 },
	/* 0x2E */ { "MVI L D8",   .execute = i8080_cpu_instruction_mvi_l_d8,   .length = 2, .nojump =  7 },
	/* 0x2F */ { "CMA",        .execute = i8080_cpu_instruction_cma,        .length = 1, .nojump =  4 },
	/* 0x30 */ { "NOP",        .execute = i8080_cpu_instruction_nop,        .length = 1, .nojump =  4 },
	/* 0x31 */ { "LXI SP D16", .execute = i8080_cpu_instruction_lxi_sp_d16, .length = 3, .nojump = 10 },
	/* 0x32 */ { "STA A16",    .execute = i8080_cpu_instruction_sta_a16,    .length = 3, .nojump = 13 },
	/* 0x33 */ { "INX SP",     .execute = i8080_cpu_instruction_inx_sp,     .length = 1, .nojump =  5 },
	/* 0x34 */ { "INR M",      .execute = i8080_cpu_instruction_inr_m,      .length = 1, .nojump = 10 },
	/* 0x35 */ { "DCR M",      .execute = i8080_cpu_instruction_dcr_m,      .length = 1, .nojump = 10 },
	/* 0x36 */ { "MVI M D8",   .execute = i8080_cpu_instruction_mvi_m_d8,   .length = 2, .nojump = 10 },
	/* 0x37 */ { "STC",        .execute = i8080_cpu_instruction_stc,        .length = 1, .nojump =  4 },
	/* 0x38 */ { "NOP",        .execute = i8080_cpu_instruction_nop,        .length = 1, .nojump =  4 },
	/* 0x39 */ { "DAD SP",     .execute = i8080_cpu_instruction_dad_sp,     .length = 1, .nojump = 10 },
	/* 0x3A */ { "LDA A16",    .execute = i8080_cpu_instruction_lda_a16,    .length = 3, .nojump = 13 },
	/* 0x3B */ { "DCX SP",     .execute = i8080_cpu_instruction_dcx_sp,     .length = 1, .nojump =  5 },
	/* 0x3C */ { "INR A",      .execute = i8080_cpu_instruction_inr_a,      .length = 1, .nojump =  5 },
	/* 0x3D */ { "DCR A",      .execute = i8080_cpu_instruction_dcr_a,      .length = 1, .nojump =  5 },
	/* 0x3E */ { "MVI A D8",   .execute = i8080_cpu_instruction_mvi_a_d8,   .length = 2, .nojump =  7 },
	/* 0x3F */ { "CMC",        .execute = i8080_cpu_instruction_cmc,        .length = 1, .nojump =  4 },
	/* 0x40 */ { "MOV B B",    .execute = i8080_cpu_instruction_mov_b_b,    .length = 1, .nojump =  5 },
	/* 0x41 */ { "MOV B C",    .execute = i8080_cpu_instruction_mov_b_c,    .length = 1, .nojump =  5 },
	/* 0x42 */ { "MOV B D",    .execute = i8080_cpu_instruction_mov_b_d,    .length = 1, .nojump =  5 },
	/* 0x43 */ { "MOV B E",    .execute = i8080_cpu_instruction_mov_b_e,    .length = 1, .nojump =  5 },
	/* 0x44 */ { "MOV B H",    .execute = i8080_cpu_instruction_mov_b_h,    .length = 1, .nojump =  5 },
	/* 0x45 */ { "MOV B L",    .execute = i8080_cpu_instruction_mov_b_l,    .length = 1, .nojump =  5 },
	/* 0x46 */ { "MOV B M",    .execute = i8080_cpu_instruction_mov_b_m,    .length = 1, .nojump =  7 },
	/* 0x47 */ { "MOV B A",    .execute = i8080_cpu_instruction_mov_b_a,    .length = 1, .nojump =  5 },
	/* 0x48 */ { "MOV C B",    .execute = i8080_cpu_instruction_mov_c_b,    .length = 1, .nojump =  5 },
	/* 0x49 */ { "MOV C C",    .execute = i8080_cpu_instruction_mov_c_c,    .length = 1, .nojump =  5 },
	/* 0x4A */ { "MOV C D",    .execute = i8080_cpu_instruction_mov_c_d,    .length = 1, .nojump =  5 },
	/* 0x4B */ { "MOV C E",    .execute = i8080_cpu_instruction_mov_c_e,    .length = 1, .nojump =  5 },
	/* 0x4C */ { "MOV C H",    .execute = i8080_cpu_instruction_mov_c_h,    .length = 1, .nojump =  5 },
	/* 0x4D */ { "MOV C L",    .execute = i8080_cpu_instruction_mov_c_l,    .length = 1, .nojump =  5 },
	/* 0x4E */ { "MOV C M",    .execute = i8080_cpu_instruction_mov_c_m,    .length = 1, .nojump =  7 },
	/* 0x4F */ { "MOV C A",    .execute = i8080_cpu_instruction_mov_c_a,    .length = 1, .nojump =  5 },
	/* 0x50 */ { "MOV D B",    .execute = i8080_cpu_instruction_mov_d_b,    .length = 1, .nojump =  5 },
	/* 0x51 */ { "MOV D C",    .execute = i8080_cpu_instruction_mov_d_c,    .length = 1, .nojump =  5 },
	/* 0x52 */ { "MOV D D",    .execute = i8080_cpu_instruction_mov_d_d,    .length = 1, .nojump =  5 },
	/* 0x53 */ { "MOV D E",    .execute = i8080_cpu_instruction_mov_d_e,    .length = 1, .nojump =  5 },
	/* 0x54 */ { "MOV D H",    .execute = i8080_cpu_instruction_mov_d_h,    .length = 1, .nojump =  5 },
	/* 0x55 */ { "MOV D L",    .execute = i8080_cpu_instruction_mov_d_l,    .length = 1, .nojump =  5 },
	/* 0x56 */ { "MOV D M",    .execute = i8080_cpu_instruction_mov_d_m,    .length = 1, .nojump =  7 },
	/* 0x57 */ { "MOV D A",    .execute = i8080_cpu_instruction_mov_d_a,    .length = 1, .nojump =  5 },
	/* 0x58 */ { "MOV E B",    .execute = i8080_cpu_instruction_mov_e_b,    .length = 1, .nojump =  5 },
	/* 0x59 */ { "MOV E C",    .execute = i8080_cpu_instruction_mov_e_c,    .length = 1, .nojump =  5 },
	/* 0x5A */ { "MOV E D",    .execute = i8080_cpu_instruction_mov_e_d,    .length = 1, .nojump =  5 },
	/* 0x5B */ { "MOV E E",    .execute = i8080_cpu_instruction_mov_e_e,    .length = 1, .nojump =  5 },
	/* 0x5C */ { "MOV E H",    .execute = i8080_cpu_instruction_mov_e_h,    .length = 1, .nojump =  5 },
	/* 0x5D */ { "MOV E L",    .execute = i8080_cpu_instruction_mov_e_l,    .length = 1, .nojump =  5 },
	/* 0x5E */ { "MOV E M",    .execute = i8080_cpu_instruction_mov_e_m,    .length = 1, .nojump =  7 },
	/* 0x5F */ { "MOV E A",    .execute = i8080_cpu_instruction_mov_e_a,    .length = 1, .nojump =  5 },
	/* 0x60 */ { "MOV H B",    .execute = i8080_cpu_instruction_mov_h_b,    .length = 1, .nojump =  5 },
	/* 0x61 */ { "MOV H C",    .execute = i8080_cpu_instruction_mov_h_c,    .length = 1, .nojump =  5 },
	/* 0x62 */ { "MOV H D",    .execute = i8080_cpu_instruction_mov_h_d,    .length = 1, .nojump =  5 },
	/* 0x63 */ { "MOV H E",    .execute = i8080_cpu_instruction_mov_h_e,    .length = 1, .nojump =  5 },
	/* 0x64 */ { "MOV H H",    .execute = i8080_cpu_instruction_mov_h_h,    .length = 1, .nojump =  5 },
	/* 0x65 */ { "MOV H L",    .execute = i8080_cpu_instruction_mov_h_l,    .length = 1, .nojump =  5 },
	/* 0x66 */ { "MOV H M",    .execute = i8080_cpu_instruction_mov_h_m,    .length = 1, .nojump =  7 },
	/* 0x67 */ { "MOV H A",    .execute = i8080_cpu_instruction_mov_h_a,    .length = 1, .nojump =  5 },
	/* 0x68 */ { "MOV L B",    .execute = i8080_cpu_instruction_mov_l_b,    .length = 1, .nojump =  5 },
	/* 0x69 */ { "MOV L C",    .execute = i8080_cpu_instruction_mov_l_c,    .length = 1, .nojump =  5 },
	/* 0x6A */ { "MOV L D",    .execute = i8080_cpu_instruction_mov_l_d,    .length = 1, .nojump =  5 },
	/* 0x6B */ { "MOV L E",    .execute = i8080_cpu_instruction_mov_l_e,    .length = 1, .nojump =  5 },
	/* 0x6C */ { "MOV L H",    .execute = i8080_cpu_instruction_mov_l_h,    .length = 1, .nojump =  5 },
	/* 0x6D */ { "MOV L L",    .execute = i8080_cpu_instruction_mov_l_l,    .length = 1, .nojump =  5 },
	/* 0x6E */ { "MOV L M",    .execute = i8080_cpu_instruction_mov_l_m,    .length = 1, .nojump =  7 },
	/* 0x6F */ { "MOV L A",    .execute = i8080_cpu_instruction_mov_l_a,    .length = 1, .nojump =  5 },
	/* 0x70 */ { "MOV M B",    .execute = i8080_cpu_instruction_mov_m_b,    .length = 1, .nojump =  7 },
	/* 0x71 */ { "MOV M C",    .execute = i8080_cpu_instruction_mov_m_c,    .length = 1, .nojump =  7 },
	/* 0x72 */ { "MOV M D",    .execute = i8080_cpu_instruction_mov_m_d,    .length = 1, .nojump =  7 },
	/* 0x73 */ { "MOV M E",    .execute = i8080_cpu_instruction_mov_m_e,    .length = 1, .nojump =  7 },
	/* 0x74 */ { "MOV M H",    .execute = i8080_cpu_instruction_mov_m_h,    .length = 1, .nojump =  7 },
	/* 0x75 */ { "MOV M L",    .execute = i8080_cpu_instruction_mov_m_l,    .length = 1, .nojump =  7 },
	/* 0x76 */ { "HLT",        .execute = i8080_cpu_instruction_hlt,        .length = 1, .nojump =  7 },
	/* 0x77 */ { "MOV M A",    .execute = i8080_cpu_instruction_mov_m_a,    .length = 1, .nojump =  7 },
	/* 0x78 */ { "MOV A B",    .execute = i8080_cpu_instruction_mov_a_b,    .length = 1, .nojump =  5 },
	/* 0x79 */ { "MOV A C",    .execute = i8080_cpu_instruction_mov_a_c,    .length = 1, .nojump =  5 },
	/* 0x7A */ { "MOV A D",    .execute = i8080_cpu_instruction_mov_a_d,    .length = 1, .nojump =  5 },
	/* 0x7B */ { "MOV A E",    .execute = i8080_cpu_instruction_mov_a_e,    .length = 1, .nojump =  5 },
	/* 0x7C */ { "MOV A H",    .execute = i8080_cpu_instruction_mov_a_h,    .length = 1, .nojump =  5 },
	/* 0x7D */ { "MOV A L",    .execute = i8080_cpu_instruction_mov_a_l,    .length = 1, .nojump =  5 },
	/* 0x7E */ { "MOV A M",    .execute = i8080_cpu_instruction_mov_a_m,    .length = 1, .nojump =  7 },
	/* 0x7F */ { "MOV A A",    .execute = i8080_cpu_instruction_mov_a_a,    .length = 1, .nojump =  5 },
	/* 0x80 */ { "ADD B",      .execute = i8080_cpu_instruction_add_b,      .length = 1, .nojump =  4 },
	/* 0x81 */ { "ADD C",      .execute = i8080_cpu_instruction_add_c,      .length = 1, .nojump =  4 },
	/* 0x82 */ { "ADD D",      .execute = i8080_cpu_instruction_add_d,      .length = 1, .nojump =  4 },
	/* 0x83 */ { "ADD E",      .execute = i8080_cpu_instruction_add_e,      .length = 1, .nojump =  4 },
	/* 0x84 */ { "ADD H",      .execute = i8080_cpu_instruction_add_h,      .length = 1, .nojump =  4 },
	/* 0x85 */ { "ADD L",      .execute = i8080_cpu_instruction_add_l,      .length = 1, .nojump =  4 },
	/* 0x86 */ { "ADD M",      .execute = i8080_cpu_instruction_add_m,      .length = 1, .nojump =  7 },
	/* 0x87 */ { "ADD A",      .execute = i8080_cpu_instruction_add_a,      .length = 1, .nojump =  4 },
	/* 0x88 */ { "ADC B",      .execute = i8080_cpu_instruction_adc_b,      .length = 1, .nojump =  4 },
	/* 0x89 */ { "ADC C",      .execute = i8080_cpu_instruction_adc_c,      .length = 1, .nojump =  4 },
	/* 0x8A */ { "ADC D",      .execute = i8080_cpu_instruction_adc_d,      .length = 1, .nojump =  4 },
	/* 0x8B */ { "ADC E",      .execute = i8080_cpu_instruction_adc_e,      .length = 1, .nojump =  4 },
	/* 0x8C */ { "ADC H",      .execute = i8080_cpu_instruction_adc_h,      .length = 1, .nojump =  4 },
	/* 0x8D */ { "ADC L",      .execute = i8080_cpu_instruction_adc_l,      .length = 1, .nojump =  4 },
	/* 0x8E */ { "ADC M",      .execute = i8080_cpu_instruction_adc_m,      .length = 1, .nojump =  7 },
	/* 0x8F */ { "ADC A",      .execute = i8080_cpu_instruction_adc_a,      .length = 1, .nojump =  4 },
	/* 0x90 */ { "SUB B",      .execute = i8080_cpu_instruction_sub_b,      .length = 1, .nojump =  4 },
	/* 0x91 */ { "SUB C",      .execute = i8080_cpu_instruction_sub_c,      .length = 1, .nojump =  4 },
	/* 0x92 */ { "SUB D",      .execute = i8080_cpu_instruction_sub_d,      .length = 1, .nojump =  4 },
	/* 0x93 */ { "SUB E",      .execute = i8080_cpu_instruction_sub_e,      .length = 1, .nojump =  4 },
	/* 0x94 */ { "SUB H",      .execute = i8080_cpu_instruction_sub_h,      .length = 1, .nojump =  4 },
	/* 0x95 */ { "SUB L",      .execute = i8080_cpu_instruction_sub_l,      .length = 1, .nojump =  4 },
	/* 0x96 */ { "SUB M",      .execute = i8080_cpu_instruction_sub_m,      .length = 1, .nojump =  7 },
	/* 0x97 */ { "SUB A",      .execute = i8080_cpu_instruction_sub_a,      .length = 1, .nojump =  4 },
	/* 0x98 */ { "SBB B",      .execute = i8080_cpu_instruction_sbb_b,      .length = 1, .nojump =  4 },
	/* 0x99 */ { "SBB C",      .execute = i8080_cpu_instruction_sbb_c,      .length = 1, .nojump =  4 },
	/* 0x9A */ { "SBB D",      .execute = i8080_cpu_instruction_sbb_d,      .length = 1, .nojump =  4 },
	/* 0x9B */ { "SBB E",      .execute = i8080_cpu_instruction_sbb_e,      .length = 1, .nojump =  4 },
	/* 0x9C */ { "SBB H",      .execute = i8080_cpu_instruction_sbb_h,      .length = 1, .nojump =  4 },
	/* 0x9D */ { "SBB L",      .execute = i8080_cpu_instruction_sbb_l,      .length = 1, .nojump =  4 },
	/* 0x9E */ { "SBB M",      .execute = i8080_cpu_instruction_sbb_m,      .length = 1, .nojump =  7 },
	/* 0x9F */ { "SBB A",      .execute = i8080_cpu_instruction_sbb_a,      .length = 1, .nojump =  4 },
	/* 0xA0 */ { "ANA B",      .execute = i8080_cpu_instruction_ana_b,      .length = 1, .nojump =  4 },
	/* 0xA1 */ { "ANA C",      .execute = i8080_cpu_instruction_ana_c,      .length = 1, .nojump =  4 },
	/* 0xA2 */ { "ANA D",      .execute = i8080_cpu_instruction_ana_d,      .length = 1, .nojump =  4 },
	/* 0xA3 */ { "ANA E",      .execute = i8080_cpu_instruction_ana_e,      .length = 1, .nojump =  4 },
	/* 0xA4 */ { "ANA H",      .execute = i8080_cpu_instruction_ana_h,      .length = 1, .nojump =  4 },
	/* 0xA5 */ { "ANA L",      .execute = i8080_cpu_instruction_ana_l,      .length = 1, .nojump =  4 },
	/* 0xA6 */ { "ANA M",      .execute = i8080_cpu_instruction_ana_m,      .length = 1, .nojump =  7 },
	/* 0xA7 */ { "ANA A",      .execute = i8080_cpu_instruction_ana_a,      .length = 1, .nojump =  4 },
	/* 0xA8 */ { "XRA B",      .execute = i8080_cpu_instruction_xra_b,      .length = 1, .nojump =  4 },
	/* 0xA9 */ { "XRA C",      .execute = i8080_cpu_instruction_xra_c,      .length = 1, .nojump =  4 },
	/* 0xAA */ { "XRA D",      .execute = i8080_cpu_instruction_xra_d,      .length = 1, .nojump =  4 },
	/* 0xAB */ { "XRA E",      .execute = i8080_cpu_instruction_xra_e,      .length = 1, .nojump =  4 },
	/* 0xAC */ { "XRA H",      .execute = i8080_cpu_instruction_xra_h,      .length = 1, .nojump =  4 },
	/* 0xAD */ { "XRA L",      .execute = i8080_cpu_instruction_xra_l,      .length = 1, .nojump =  4 },
	/* 0xAE */ { "XRA M",      .execute = i8080_cpu_instruction_xra_m,      .length = 1, .nojump =  7 },
	/* 0xAF */ { "XRA A",      .execute = i8080_cpu_instruction_xra_a,      .length = 1, .nojump =  4 },
	/* 0xB0 */ { "ORA B",      .execute = i8080_cpu_instruction_ora_b,      .length = 1, .nojump =  4 },
	/* 0xB1 */ { "ORA C",      .execute = i8080_cpu_instruction_ora_c,      .length = 1, .nojump =  4 },
	/* 0xB2 */ { "ORA D",      .execute = i8080_cpu_instruction_ora_d,      .length = 1, .nojump =  4 },
	/* 0xB3 */ { "ORA E",      .execute = i8080_cpu_instruction_ora_e,      .length = 1, .nojump =  4 },
	/* 0xB4 */ { "ORA H",      .execute = i8080_cpu_instruction_ora_h,      .length = 1, .nojump =  4 },
	/* 0xB5 */ { "ORA L",      .execute = i8080_cpu_instruction_ora_l,      .length = 1, .nojump =  4 },
	/* 0xB6 */ { "ORA M",      .execute = i8080_cpu_instruction_ora_m,      .length = 1, .nojump =  7 },
	/* 0xB7 */ { "ORA A",      .execute = i8080_cpu_instruction_ora_a,      .length = 1, .nojump =  4 },
	/* 0xB8 */ { "CMP B",      .execute = i8080_cpu_instruction_cmp_b,      .length = 1, .nojump =  4 },
	/* 0xB9 */ { "CMP C",      .execute = i8080_cpu_instruction_cmp_c,      .length = 1, .nojump =  4 },
	/* 0xBA */ { "CMP D",      .execute = i8080_cpu_instruction_cmp_d,      .length = 1, .nojump =  4 },
	/* 0xBB */ { "CMP E",      .execute = i8080_cpu_instruction_cmp_e,      .length = 1, .nojump =  4 },
	/* 0xBC */ { "CMP H",      .execute = i8080_cpu_instruction_cmp_h,      .length = 1, .nojump =  4 },
	/* 0xBD */ { "CMP L",      .execute = i8080_cpu_instruction_cmp_l,      .length = 1, .nojump =  4 },
	/* 0xBE */ { "CMP M",      .execute = i8080_cpu_instruction_cmp_m,      .length = 1, .nojump =  7 },
	/* 0xBF */ { "CMP A",      .execute = i8080_cpu_instruction_cmp_a,      .length = 1, .nojump =  4 },
	/* 0xC0 */ { "RNZ",        .execute = i8080_cpu_instruction_rnz,        .length = 1, .nojump =  5, .onjump = 11 },
	/* 0xC1 */ { "POP B",      .execute = i8080_cpu_instruction_pop_b,      .length = 1, .nojump = 10 },
	/* 0xC2 */ { "JNZ A16",    .execute = i8080_cpu_instruction_jnz_a16,    .length = 3, .nojump = 10, .onjump = 10 },
	/* 0xC3 */ { "JMP A16",    .execute = i8080_cpu_instruction_jmp_a16,    .length = 3, .onjump = 10 },
	/* 0xC4 */ { "CNZ A16",    .execute = i8080_cpu_instruction_cnz_a16,    .length = 3, .nojump = 11, .onjump = 17 },
	/* 0xC5 */ { "PUSH B",     .execute = i8080_cpu_instruction_push_b,     .length = 1, .nojump = 11 },
	/* 0xC6 */ { "ADI D8",     .execute = i8080_cpu_instruction_adi_d8,     .length = 2, .nojump =  7 },
	/* 0xC7 */ { "RST 0",      .execute = i8080_cpu_instruction_rst_0,      .length = 1, .onjump = 11 },
	/* 0xC8 */ { "RZ",         .execute = i8080_cpu_instruction_rz,         .length = 1, .nojump =  5, .onjump = 11 },
	/* 0xC9 */ { "RET",        .execute = i8080_cpu_instruction_ret,        .length = 1, .onjump = 10 },
	/* 0xCA */ { "JZ A16",     .execute = i8080_cpu_instruction_jz_a16,     .length = 3, .nojump = 10, .onjump = 10 },
	/* 0xCB */ { "JMP A16",    .execute = i8080_cpu_instruction_jmp_a16,    .length = 3, .onjump = 10 },
	/* 0xCC */ { "CZ A16",     .execute = i8080_cpu_instruction_cz_a16,     .length = 3, .nojump = 11, .onjump = 17 },
	/* 0xCD */ { "CALL A16",   .execute = i8080_cpu_instruction_call_a16,   .length = 3, .onjump = 17 },
	/* 0xCE */ { "ACI D8",     .execute = i8080_cpu_instruction_aci_d8,     .length = 2, .nojump =  7 },
	/* 0xCF */ { "RST 1",      .execute = i8080_cpu_instruction_rst_1,      .length = 1, .onjump = 11 },
	/* 0xD0 */ { "RNC",        .execute = i8080_cpu_instruction_rnc,        .length = 1, .nojump =  5, .onjump = 11 },
	/* 0xD1 */ { "POP D",      .execute = i8080_cpu_instruction_pop_d,      .length = 1, .nojump = 10 },
	/* 0xD2 */ { "JNC A16",    .execute = i8080_cpu_instruction_jnc_a16,    .length = 3, .nojump = 10, .onjump = 10 },
	/* 0xD3 */ { "OUT D8",     .execute = i8080_cpu_instruction_out_d8,     .length = 2, .nojump = 10 },
	/* 0xD4 */ { "CNC A16",    .execute = i8080_cpu_instruction_cnc_a16,    .length = 3, .nojump = 11, .onjump = 17 },
	/* 0xD5 */ { "PUSH D",     .execute = i8080_cpu_instruction_push_d,     .length = 1, .nojump = 11 },
	/* 0xD6 */ { "SUI D8",     .execute = i8080_cpu_instruction_sui_d8,     .length = 2, .nojump = 7 },
	/* 0xD7 */ { "RST 2",      .execute = i8080_cpu_instruction_rst_2,      .length = 1, .onjump = 11 },
	/* 0xD8 */ { "RC",         .execute = i8080_cpu_instruction_rc,         .length = 1, .nojump =  5, .onjump = 11 },
	/* 0xD9 */ { "RET",        .execute = i8080_cpu_instruction_ret,        .length = 1, .onjump = 10 },
	/* 0xDA */ { "JC A16",     .execute = i8080_cpu_instruction_jc_a16,     .length = 3, .nojump = 10, .onjump = 10 },
	/* 0xDB */ { "IN D8",      .execute = i8080_cpu_instruction_in_d8,      .length = 2, .nojump = 10 },
	/* 0xDC */ { "CC A16",     .execute = i8080_cpu_instruction_cc_a16,     .length = 3, .nojump = 11, .onjump = 17 },
	/* 0xDD */ { "CALL A16",   .execute = i8080_cpu_instruction_call_a16,   .length = 3, .onjump = 17 },
	/* 0xDE */ { "SBI D8",     .execute = i8080_cpu_instruction_sbi_d8,     .length = 2, .nojump =  7 },
	/* 0xDF */ { "RST 3",      .execute = i8080_cpu_instruction_rst_3,      .length = 1, .onjump = 11 },
	/* 0xE0 */ { "RPO",        .execute = i8080_cpu_instruction_rpo,        .length = 1, .nojump =  5, .onjump = 11 },
	/* 0xE1 */ { "POP H",      .execute = i8080_cpu_instruction_pop_h,      .length = 1, .nojump = 10 },
	/* 0xE2 */ { "JPO A16",    .execute = i8080_cpu_instruction_jpo_a16,    .length = 3, .nojump = 10, .onjump = 10 },
	/* 0xE3 */ { "XTHL",       .execute = i8080_cpu_instruction_xthl,       .length = 1, .nojump = 18 },
	/* 0xE4 */ { "CPO A16",    .execute = i8080_cpu_instruction_cpo_a16,    .length = 3, .nojump = 11, .onjump = 17 },
	/* 0xE5 */ { "PUSH H",     .execute = i8080_cpu_instruction_push_h,     .length = 1, .nojump = 11 },
	/* 0xE6 */ { "ANI D8",     .execute = i8080_cpu_instruction_ani_d8,     .length = 2, .nojump =  7 },
	/* 0xE7 */ { "RST 4",      .execute = i8080_cpu_instruction_rst_4,      .length = 1, .onjump = 11 },
	/* 0xE8 */ { "RPE",        .execute = i8080_cpu_instruction_rpe,        .length = 1, .nojump =  5, .onjump = 11 },
	/* 0xE9 */ { "PCHL",       .execute = i8080_cpu_instruction_pchl,       .length = 1, .onjump =  5 },
	/* 0xEA */ { "JPE A16",    .execute = i8080_cpu_instruction_jpe_a16,    .length = 3, .nojump = 10, .onjump = 10 },
	/* 0xEB */ { "XCHG",       .execute = i8080_cpu_instruction_xchg,       .length = 1, .nojump =  5 },
	/* 0xEC */ { "CPE A16",    .execute = i8080_cpu_instruction_cpe_a16,    .length = 3, .nojump = 11, .onjump = 17 },
	/* 0xED */ { "CALL A16",   .execute = i8080_cpu_instruction_call_a16,   .length = 3, .onjump = 17 },
	/* 0xEE */ { "XRI D8",     .execute = i8080_cpu_instruction_xri_d8,     .length = 2, .nojump =  7 },
	/* 0xEF */ { "RST 5",      .execute = i8080_cpu_instruction_rst_5,      .length = 1, .onjump = 11 },
	/* 0xF0 */ { "RP",         .execute = i8080_cpu_instruction_rp,         .length = 1, .nojump =  5, .onjump = 11 },
	/* 0xF1 */ { "POP PSW",    .execute = i8080_cpu_instruction_pop_psw,    .length = 1, .nojump = 10 },
	/* 0xF2 */ { "JP A16",     .execute = i8080_cpu_instruction_jp_a16,     .length = 3, .nojump = 10, .onjump = 10 },
	/* 0xF3 */ { "DI",         .execute = i8080_cpu_instruction_di,         .length = 1, .nojump =  4 },
	/* 0xF4 */ { "CP A16",     .execute = i8080_cpu_instruction_cp_a16,     .length = 3, .nojump = 11, .onjump = 17 },
	/* 0xF5 */ { "PUSH PSW",   .execute = i8080_cpu_instruction_push_psw,   .length = 1, .nojump = 11 },
	/* 0xF6 */ { "ORI D8",     .execute = i8080_cpu_instruction_ori_d8,     .length = 2, .nojump =  7 },
	/* 0xF7 */ { "RST 6",      .execute = i8080_cpu_instruction_rst_6,      .length = 1, .onjump = 11 },
	/* 0xF8 */ { "RM",         .execute = i8080_cpu_instruction_rm,         .length = 1, .nojump =  5, .onjump = 11 },
	/* 0xF9 */ { "SPHL",       .execute = i8080_cpu_instruction_sphl,       .length = 1, .nojump =  5 },
	/* 0xFA */ { "JM A16",     .execute = i8080_cpu_instruction_jm_a16,     .length = 3, .nojump = 10, .onjump = 10 },
	/* 0xFB */ { "EI",         .execute = i8080_cpu_instruction_ei,         .length = 1, .nojump =  4 },
	/* 0xFC */ { "CM A16",     .execute = i8080_cpu_instruction_cm_a16,     .length = 3, .nojump = 11, .onjump = 17 },
	/* 0xFD */ { "CALL A16",   .execute = i8080_cpu_instruction_call_a16,   .length = 3, .onjump = 17 },
	/* 0xFE */ { "CPI D8",     .execute = i8080_cpu_instruction_cpi_d8,     .length = 2, .nojump =  7 },
	/* 0xFF */ { "RST 7",      .execute = i8080_cpu_instruction_rst_7,      .length = 1, .onjump = 11 },
};

int
i8080_cpu_init(struct i8080_cpu *cpu, const struct i8080_io *io) {

	memset(cpu, 0, sizeof(*cpu));

	cpu->registers.f = I8080_MASK_CONDITION_UNUSED1;
	cpu->inte = 1;
	cpu->io = io;

	return 0;
}

int
i8080_cpu_deinit(struct i8080_cpu *cpu) {
	return 0;
}

int
i8080_cpu_next(struct i8080_cpu *cpu) {
	const struct i8080_instruction *instruction = instructions + cpu->memory[cpu->pc];
	union i8080_imm imm = { };

	if(cpu->stopped) {
		return 0;
	}

	switch(instruction->length) {
	case 2:
		i8080_cpu_load8(cpu, cpu->pc + 1, &imm.d8);
		break;
	case 3:
		i8080_cpu_load16(cpu, cpu->pc + 1, &imm.d16);
		break;
	default:
		break;
	}

	cpu->pc += instruction->length;

	if(!instruction->execute(cpu, imm)) { /* nojump */
		cpu->uptime_cycles += instruction->nojump;
	} else { /* onjump */
		cpu->uptime_cycles += instruction->onjump;
	}

	return 0;
}

int
i8080_cpu_interrupt(struct i8080_cpu *cpu, uint8_t opcode, union i8080_imm imm) {
	const struct i8080_instruction *instruction = instructions + opcode;

	if(!cpu->inte) {
		return 0;
	}

	cpu->pc += instruction->length;
	cpu->inte = 0;

	if(!instruction->execute(cpu, imm)) { /* nojump */
		cpu->uptime_cycles += instruction->nojump;
	} else { /* onjump */
		cpu->uptime_cycles += instruction->onjump;
	}

	return 0;
}

const struct i8080_instruction *
i8080_instruction_info(uint8_t opcode) {
	return instructions + opcode;
}

