#pragma once
#include "pch.h"

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