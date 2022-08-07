#pragma once
#include "pch.h"


// MIPS exceptions, traps, interrupts 

constexpr uint32_t EXCEPTION_HANDLER = 0x80000180;

enum EXCEPTION_TYPES : uint32_t {
	INVALID_EXCEPTION = -1,
	INTERRUPT_EXCEPTION = 0,
	ADDRESS_EXCEPTION_LOAD = 4, 
	ADDRESS_EXCEPTION_STORE = 5, 
	SYSCALL_EXCEPTION = 8, 
	BREAKPOINT_EXCEPTION = 9, 
	RESERVED_INSTRUCTION_EXCEPTION = 10, 
	ARITHMETIC_OVERFLOW_EXCEPTION = 12, 
	TRAP_EXCEPTION = 13, 
	DIVIDE_BY_ZERO_EXCEPTION = 15, 
	FLOATING_POINT_OVERFLOW = 16, 
	FLOATING_POINT_UNDERFLOW = 17
};

// Base class for all exceptions that can get handled by the kernelmode exception handler
class mips_exception : public std::exception {
public:
	mips_exception(std::string reason): std::exception(reason.c_str()) {}

	virtual uint32_t exception_type() const {
		return INVALID_EXCEPTION;
	}

	virtual uint32_t get_vaddr() const {
		return 0;
	}

	virtual bool invalid_memory_address() const {
		return false;
	}
};

class mips_exception_memory : public mips_exception {
public:
	mips_exception_memory(std::string reason, uint32_t addr) : mips_exception(reason), m_vaddr(addr) {}

	virtual uint32_t get_vaddr() const override {
		return m_vaddr;
	}

	virtual bool invalid_memory_address() const override {
		return true;
	}

private:
	uint32_t m_vaddr;
};

class mips_exception_load : public mips_exception_memory {
public:
	mips_exception_load(std::string reason, uint32_t addr) : mips_exception_memory(reason, addr) {}

	virtual uint32_t exception_type() const override {
		return ADDRESS_EXCEPTION_LOAD;
	}
};

class mips_exception_store : public mips_exception_memory {
public:
	mips_exception_store(std::string reason, uint32_t addr) : mips_exception_memory(reason, addr) {}

	virtual uint32_t exception_type() const override {
		return ADDRESS_EXCEPTION_STORE;
	}
};

class mips_exception_interrupt : public mips_exception {
public:
	mips_exception_interrupt(std::string reason): mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return (1 << 6) | INTERRUPT_EXCEPTION; // the caller will leftshift by 2, which would make the 8th bit set (and exception number 0)
	}
};

class mips_exception_syscall : public mips_exception {
public:
	mips_exception_syscall(std::string reason) : mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return SYSCALL_EXCEPTION;
	}
};

class mips_exception_breakpoint : public mips_exception {
public:
	mips_exception_breakpoint(std::string reason) : mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return BREAKPOINT_EXCEPTION;
	}
};

class mips_exception_reserved_instruction : public mips_exception {
public:
	mips_exception_reserved_instruction(std::string reason) : mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return RESERVED_INSTRUCTION_EXCEPTION;
	}
};

class mips_exception_arithmetic_overflow : public mips_exception {
public:
	mips_exception_arithmetic_overflow(std::string reason) : mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return ARITHMETIC_OVERFLOW_EXCEPTION;
	}
};

class mips_exception_trap : public mips_exception {
public:
	mips_exception_trap(std::string reason) : mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return TRAP_EXCEPTION;
	}
};

class mips_exception_zero_division : public mips_exception {
public:
	mips_exception_zero_division(std::string reason) : mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return DIVIDE_BY_ZERO_EXCEPTION;
	}
};

class mips_exception_float_overflow : public mips_exception {
public:
	mips_exception_float_overflow(std::string reason) : mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return FLOATING_POINT_OVERFLOW;
	}
};

class mips_exception_float_underflow : public mips_exception {
public:
	mips_exception_float_underflow(std::string reason) : mips_exception(reason) {}

	virtual uint32_t exception_type() const override {
		return FLOATING_POINT_UNDERFLOW;
	}
};

// Shouldn't be handled by kernelmode exception handler.
class mips_exception_exit : public std::exception {
public:
	mips_exception_exit(std::string reason = "EXIT syscall invoked") : std::exception(reason.c_str()) {}
};