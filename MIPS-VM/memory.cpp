#include "pch.h"
#include "memory.h"

#define STACK_SIZE 0x20000000 // about 500 mb
#define STACK_TOP 0x7FFFEFFF
#define STACK_BOTTOM STACK_TOP - STACK_SIZE

stack::stack() {
	m_stack.address = STACK_TOP; // stack "end"
	m_stack.sect.resize(STACK_SIZE, 0);
}

stack::~stack() {

}

section* stack::get_section_if_valid_stack(uint32_t addr) {
	if (addr >= STACK_BOTTOM && addr < STACK_TOP) {
		return &m_stack;
	}

	return nullptr;
}

bool stack::is_safe_access(uint32_t addr, uint32_t size) {
	if (addr < STACK_BOTTOM || addr >= STACK_TOP ||
		addr + size < STACK_BOTTOM || addr + size > STACK_TOP) {
		return false;
	}

	return true;
}


#define SBRK_HEAP_START 0x01000000
#define SBRK_HEAP_END 0x04000000

heap::heap() {
	m_heap_offset = 0;

	m_heap.address = SBRK_HEAP_START;
	m_heap.sect.resize(SBRK_HEAP_END - SBRK_HEAP_START, 0);
}

heap::~heap() {

}

uint32_t heap::sbrk(int32_t bytes) {
	if (bytes < 0) {
		throw std::runtime_error("Can't allocate negative amount of bytes with sbrk");
	}
	if (bytes > m_heap.sect.size() - m_heap_offset) {
		throw std::runtime_error("Out of heap memory for sbrk");
	}

	uint32_t addr = SBRK_HEAP_START + m_heap_offset;
	m_heap_offset += bytes;

	return addr;
}

section* heap::get_section_if_valid_heap(uint32_t addr) {
	if (addr >= SBRK_HEAP_START && addr < SBRK_HEAP_END) {
		return &m_heap;
	}

	return nullptr;
}

bool heap::is_safe_access(uint32_t addr, uint32_t size) {
	if (addr < SBRK_HEAP_START || addr >= SBRK_HEAP_END ||
		addr + size < SBRK_HEAP_START || addr + size > SBRK_HEAP_END) {
		return false;
	}

	return true;
}