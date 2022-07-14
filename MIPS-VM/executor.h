#pragma once
#include "pch.h"
#include "registers.h"
#include "instruction.h"
#include "memory.h"


class executor {
public:
	executor(std::string file);
	~executor() {}

	void run();
	bool can_run() { return m_can_run; }
private:

	bool dispatch(instruction inst);
	bool dispatch_funct(instruction inst);
	bool dispatch_syscall();

	uint32_t get_offset_for_section(section* sect, uint32_t addr);
	section* get_section_for_address(uint32_t addr);
	bool is_safe_access(section* sect, uint32_t addr, uint32_t size);

	registers m_regs;

	section m_data;
	section m_text;
	section m_ktext;

	heap m_heap;
	stack m_stack;

	bool m_can_run;
};