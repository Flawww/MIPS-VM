#pragma once
#include "pch.h"

enum class register_names : int {
	zero = 0, 
	at, v0, v1, a0, a1, a2, a3,
	t0, t1, t2, t3, t4, t5, t6, t7,
	s0, s1, s2, s3, s4, s5, s6, s7,
	t8, t9,
	k0, k1,
	gp, sp, fp, ra
};

struct registers {
	registers() {
		memset(this, 0, sizeof(registers));
	}

	uint32_t regs[32];
	uint32_t pc;
	uint32_t hi;
	uint32_t lo;

	// coproc 1
	union {
		float f[32];
		double d[16];
	};

	// coproc 0
	uint32_t vaddr; // $8
	uint32_t status; // $12
	uint32_t cause; // $13
	uint32_t epc; // $14
};