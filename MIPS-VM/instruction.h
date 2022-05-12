#pragma once
#include "pch.h"

struct instruction {
	instruction(uint32_t hex): hex(hex) {}

	struct r_format {
		uint32_t funct : 6;
		uint32_t shift : 5;
		uint32_t rd : 5;
		uint32_t rt : 5;
		uint32_t rs : 5;
		uint32_t opcode : 6;
	};

	struct i_format {
		uint32_t imm : 16;
		uint32_t rt : 5;
		uint32_t rs : 5;
		uint32_t opcode : 6;	
	};

	struct j_format {
		uint32_t p_addr : 26; // pseudo address
		uint32_t opcode : 6;
	};

	union {
		r_format r;
		i_format i;
		j_format j;
		uint32_t hex;
	};
};