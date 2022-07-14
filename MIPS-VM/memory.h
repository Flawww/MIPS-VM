#pragma once
#include "pch.h"

struct section {
	section() : address(0) {}

	uint32_t address;
	std::vector<uint8_t> sect;
};

// stack has a maximum size of 500mbs
class stack {
public:
	stack();
	~stack();

	section* get_section_if_valid_stack(uint32_t addr);
	bool is_safe_access(uint32_t addr, uint32_t size);

private:

	section m_stack;
};

// heap has a maximum size of about 50mbs
class heap {
public:
	heap();
	~heap();

	uint32_t sbrk(int32_t bytes);
	section* get_section_if_valid_heap(uint32_t addr);
	bool is_safe_access(uint32_t addr, uint32_t size);
	
private:

	uint32_t m_heap_offset;
	section m_heap;
};