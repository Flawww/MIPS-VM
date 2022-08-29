#pragma once
#include "pch.h"
#include "registers.h"

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

	void push_syscall_frame(registers& regs) {
		syscall_frame frame;
		frame.status = regs.status;
		frame.cause = regs.cause;
		frame.epc = regs.epc;
		m_syscall_frames.push(frame);
	}

	bool pop_syscall_frame(registers& regs) {
		if (m_syscall_frames.empty()) {
			return false;
		}

		syscall_frame frame = m_syscall_frames.top();
		regs.status = frame.status;
		regs.cause = frame.cause;
		regs.epc = frame.epc;

		m_syscall_frames.pop();
		return true;
	}

private:
	struct syscall_frame {
		uint32_t status; // $12
		uint32_t cause; // $13
		uint32_t epc; // $14
	};

	std::unordered_map<uint32_t, uint32_t> m_registered_syscalls;
	std::stack<syscall_frame> m_syscall_frames;
};