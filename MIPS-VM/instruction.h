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

enum class instructions : int {
	// R FORMAT
	R_FORMAT = 0x00, // uses funct
	ERET = 0x10, // uses funct
	MFC0 = 0x10,
	MFC1 = 0x11,
	MUL = 0x1C,

	// J FORMAT
	J = 0x02,
	JAL = 0x03,

	// I FORMAT
	TRAPI = 0x01,
	SLTI = 0x0A,
	SLTIU = 0x0B,
	ANDI = 0x0C,
	ORI = 0x0D,
	LUI = 0x0F,
	SW = 0x2B,
	BEQ = 0x04,
	BNE = 0x05,
	BLEZ = 0x06,
	BGTZ = 0x07,
	ADDI = 0x08,
	ADDIU = 0x09,
	LB = 0x20,
	LH = 0x21,
	LW = 0x23,
	LBU = 0x24,
	LHU = 0x25,
	SB = 0x28,
	SH = 0x29,
};

enum class funct : int {
	SYSCALL = 0x0C,
	BREAK = 0x0D,
	SLL = 0x00,
	DIV = 0x1A,
	DIVU = 0x1B,
	SRL = 0x02,
	SLT = 0x2A,
	SLTU = 0x2B,
	SRA = 0x03,
	JR = 0x08,
	JALR = 0x09,
	MFHI = 0x10,
	MTHI = 0x11,
	MFLO = 0x12,
	MTLO = 0x13,
	MULT = 0x18,
	MULTU = 0x19,
	ADD = 0x20,
	ADDU = 0x21,
	SUB = 0x22,
	SUBU = 0x23,
	AND = 0x24,
	OR = 0x25,
	XOR = 0x26,
	NOR = 0x27,
	TGE = 0x30,
	TGEU = 0x31,
	TLT = 0x32,
	TLTU = 0x33,
	TEQ = 0x34,
	TNE = 0x36,
};

enum class syscalls : int {
	PRINT_INT = 1,
	PRINT_FLOAT = 2,
	PRINT_DBL = 3,
	PRINT_STRING = 4,
	READ_INT = 5,
	READ_FLOAT = 6,
	READ_DBL = 7,
	READ_STRING = 8,
	SBRK = 9,
	EXIT = 10,
	PRINT_CHAR = 11,
	READ_CHAR = 12,
	OPEN_FILE = 13,
	READ_FILE = 14,
	WRITE_FILE = 15,
	CLOSE_FILE = 16,
	EXIT2 = 17,
	TIME,
	SLEEP = 32,
	PRINT_HEX = 34,
	PRINT_BINARY = 35,
	PRINT_UNSIGNED = 36,
	SET_SEED = 40,
	RAND_INT = 41,
	RAND_INT_RANGE = 42,
	RAND_FLOAT = 43,
	RAND_DBL = 44,
	REGISTER_SYSCALL = 49,
};

// Stored in the "rt" bits for I-format trap instructions
enum class imm_trap_instructions : int {
	TGEI = 8,
	TGEIU = 9,
	TLTI = 10,
	TLTIU = 11,
	TEQI = 12,
	TNEI = 14,
};