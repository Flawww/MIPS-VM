#pragma once
#include "pch.h"
#include "registers.h"
#include "instruction.h"

struct section {
	section() : address(0) {}

	uint32_t address;
	std::vector<uint8_t> sect;
};

class executor {
public:
	executor(std::string file);
	~executor() {}

	void run();
	bool can_run() { return m_can_run; }
private:

	section* get_section_for_address(uint32_t addr);
	
	registers m_regs;

	section m_data;
	section m_text;
	section m_ktext;

	bool m_can_run;
};