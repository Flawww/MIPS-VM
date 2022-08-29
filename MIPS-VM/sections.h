#pragma once
#include "pch.h"

enum SECTION_FLAGS : int {
	EXECUTABLE = (1 << 0),
	MUTABLE = (1 << 1),
	KERNEL = (1 << 2),
};

enum SECTIONS : int {
	TEXT = 0,
	DATA,
	KTEXT,
	KDATA,

	NUM_SECTIONS
};

constexpr const char* section_names[] = {
	".text", ".data", ".ktext", ".kdata"
};

constexpr int section_protection[] = {
	// .text	.data			.ktext			  .kdata
	EXECUTABLE, MUTABLE, EXECUTABLE | KERNEL, MUTABLE | KERNEL
};

struct section {
	section() : address(0), flags(MUTABLE) {}
	section(int32_t flag) : address(0), flags(flag) {}

	uint32_t address;
	std::vector<uint8_t> sect;
	int32_t flags;
};