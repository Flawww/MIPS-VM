#pragma once
#include "pch.h"

class custom_syscall_mgr {
public:
	void register_syscall(uint32_t code, uint32_t addr) {
		m_registered_syscalls[code] = addr;
	}

	uint32_t get_syscall_addr(uint32_t code) {
		auto syscall = m_registered_syscalls.find(code);
		if (syscall == m_registered_syscalls.end()) {
			return 0;
		}

		return syscall->second;
	}

private:

	std::unordered_map<uint32_t, uint32_t> m_registered_syscalls;
};