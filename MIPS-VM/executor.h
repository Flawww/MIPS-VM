#pragma once
#include "pch.h"
#include "registers.h"
#include "instruction.h"
#include "memory.h"
#include "exceptions.h"
#include "file_mgr.h"
#include "random_mgr.h"
#include "custom_syscall_mgr.h"

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

	void keyboard_interrupt();

	uint32_t get_offset_for_section(section* sect, uint32_t addr);
	section* get_section_for_address(uint32_t addr, bool kernelmode_override = false);
	bool is_safe_access(section* sect, uint32_t addr, uint32_t size);

	registers m_regs;

	std::array<section, NUM_SECTIONS> m_sections;

	section m_mmio;

	heap m_heap;
	stack m_stack;

	uint32_t m_tick;

	custom_syscall_mgr m_syscall_mgr;
	random_mgr m_random_mgr;
	file_manager m_file_mgr;

	bool m_has_exception_handler;
	bool m_kernelmode;
	bool m_can_run;
};