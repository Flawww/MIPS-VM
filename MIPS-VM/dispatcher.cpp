#include "pch.h"
#include "executor.h"
#include "helper.h"
#include "file_mgr.h"

bool executor::dispatch(instruction inst) {
    // get the opcode (opcode is always the first 6 bits, so what format we get the opcode as does not matter)
    switch (inst.r.opcode) {
    case uint32_t(instructions::R_FORMAT):
    {
        return dispatch_funct(inst);
    }
    break;
    case uint32_t(instructions::TRAPI):
    {
        switch (inst.i.rt) {
        case uint32_t(imm_trap_instructions::TGEI):
        {
            if (int32_t(m_regs.regs[inst.i.rs]) >= bit_cast<int16_t>(inst.i.imm)) {
                throw mips_exception_trap("Trap exception");
            }
        }
        break;
        case uint32_t(imm_trap_instructions::TGEIU):
        {
            if (m_regs.regs[inst.i.rs] >= inst.i.imm) {
                throw mips_exception_trap("Trap exception");
            }
        }
        break;
        case uint32_t(imm_trap_instructions::TLTI):
        {
            if (int32_t(m_regs.regs[inst.i.rs]) < bit_cast<int16_t>(inst.i.imm)) {
                throw mips_exception_trap("Trap exception");
            }
        }
        break;
        case uint32_t(imm_trap_instructions::TLTIU):
        {
            if (m_regs.regs[inst.i.rs] < inst.i.imm) {
                throw mips_exception_trap("Trap exception");
            }
        }
        break;
        case uint32_t(imm_trap_instructions::TEQI):
        {
            if (m_regs.regs[inst.i.rs] == inst.i.imm) {
                throw mips_exception_trap("Trap exception");
            }
        }
        break;
        case uint32_t(imm_trap_instructions::TNEI):
        {
            if (m_regs.regs[inst.i.rs] != inst.i.imm) {
                throw mips_exception_trap("Trap exception");
            }
        }
        break;
        default:
            throw std::runtime_error("Unkown trap instructions");
        }
    }
    break;
    //case uint32_t(instructions::ERET): // ERET has same opcode as MFC0
    case uint32_t(instructions::MFC0):
    {
        if (inst.r.funct == 0x18) { // ERET (funct 0x18)
            m_regs.pc = m_regs.epc; // go back to usermode
            m_kernelmode = false;
            return false;
        }

        // rs - which operation to do from c0 (move to or from)
        // rt - register index
        // rd - coproc0 index

        uint32_t* c0_reg = nullptr;
        switch (inst.r.rd) {
        case 8:
            c0_reg = &m_regs.vaddr;
            break;
        case 12:
            c0_reg = &m_regs.status;
            break;
        case 13:
            c0_reg = &m_regs.cause;
            break;
        case 14:
            c0_reg = &m_regs.epc;
            break;
        default:
            throw std::runtime_error("Invalid coproc0 register index for MC0 instruction");
        }

        switch (inst.r.rs) {
        case 0:
            m_regs.regs[inst.r.rt] = *c0_reg;
            break;
        case 4:
            *c0_reg = m_regs.regs[inst.r.rt];
            break;
        default:
            throw std::runtime_error("Invalid MC0 operation");
        }

    }
    break;
    case uint32_t(instructions::MFC1):
    {
        switch (inst.r.rs) {
        case 0:
            m_regs.regs[inst.r.rt] = m_regs.f[inst.r.rd];
            break;
        case 4:
            m_regs.f[inst.r.rd] = m_regs.regs[inst.r.rt];
            break;
        default:
            throw std::runtime_error("Invalid MC1 operation");
        }

    }
    break;
    case uint32_t(instructions::MUL):
    {
        m_regs.regs[inst.r.rd] = m_regs.regs[inst.r.rs] * m_regs.regs[inst.r.rt];
    }
    break;
    case uint32_t(instructions::J):
    {
        uint32_t addr = inst.j.p_addr * 4; // lowest 28 bits are "direct"
        addr |= (m_regs.pc + 4) & 0xF0000000; // Maintain the 4 upper bits of our current address-space 
        m_regs.pc = addr;

        return false; // dont advance pc
    }
    break;
    case uint32_t(instructions::JAL):
    {
        m_regs.regs[int(register_names::ra)] = m_regs.pc + 0x4;

        uint32_t addr = inst.j.p_addr * 4; // lowest 28 bits are "direct"
        addr |= (m_regs.pc + 4) & 0xF0000000; // Maintain the 4 upper bits of our current address-space 
        m_regs.pc = addr;

        return false; // dont advance pc
    }
    break;
    case uint32_t(instructions::SLTI):
    {
        m_regs.regs[inst.i.rt] = (int32_t(m_regs.regs[inst.i.rs]) < bit_cast<int16_t>(inst.i.imm));
    }
    break;
    case uint32_t(instructions::SLTIU):
    {
        m_regs.regs[inst.i.rt] = (m_regs.regs[inst.i.rs] < inst.i.imm);
    }
    break;
    case uint32_t(instructions::ANDI):
    {
        m_regs.regs[inst.i.rt] = m_regs.regs[inst.i.rs] & inst.i.imm;
    }
    break;
    case uint32_t(instructions::ORI):
    {
        m_regs.regs[inst.i.rt] = m_regs.regs[inst.i.rs] | inst.i.imm;
    }
    break;
    case uint32_t(instructions::LUI):
    {
        m_regs.regs[inst.i.rt] = inst.i.imm << 16;
    }
    break;
    case uint32_t(instructions::BEQ):
    {
        if (m_regs.regs[inst.i.rs] == m_regs.regs[inst.i.rt]) {
            m_regs.pc += 4 + bit_cast<int16_t>(inst.i.imm) * 4;

            return false; // dont advance pc
        }
    }
    break;
    case uint32_t(instructions::BNE):
    {
        if (m_regs.regs[inst.i.rs] != m_regs.regs[inst.i.rt]) {
            m_regs.pc += 4 + bit_cast<int16_t>(inst.i.imm) * 4;

            return false; // dont advance pc
        }
    }
    break;
    case uint32_t(instructions::BLEZ):
    {
        if (int32_t(m_regs.regs[inst.i.rs]) <= 0) {
            m_regs.pc += 4 + bit_cast<int16_t>(inst.i.imm) * 4;

            return false; // dont advance pc
        }
    }
    break;
    case uint32_t(instructions::BGTZ):
    {
        if (int32_t(m_regs.regs[inst.i.rs]) > 0) {
            m_regs.pc += 4 + bit_cast<int16_t>(inst.i.imm) * 4;

            return false; // dont advance pc
        }
    }
    break;
    case uint32_t(instructions::ADDI):
    {
        int32_t a = m_regs.regs[inst.i.rs];
        int32_t b = bit_cast<int16_t>(inst.i.imm);
        // check for overflow
        if ((b > 0 && a > std::numeric_limits<int32_t>::max() - b) || (b < 0 && a < std::numeric_limits<int32_t>::min() - b)) {
            throw mips_exception_arithmetic_overflow("ADDI operation overflowed");
        }

        m_regs.regs[inst.i.rt] = a + b;
    }
    break;
    case uint32_t(instructions::ADDIU):
    {
        m_regs.regs[inst.i.rt] = m_regs.regs[inst.i.rs] + inst.i.imm;
    }
    break;
    case uint32_t(instructions::LW):
    {
        uint32_t addr = m_regs.regs[inst.i.rs] + bit_cast<int16_t>(inst.i.imm);

        section* sect = nullptr;
        if (!(sect = get_section_for_address(addr)) || !is_safe_access(sect, addr, sizeof(uint32_t))) {
            throw mips_exception_load("Invalid memory access for LW operation", addr);
        }

        uint32_t offset = get_offset_for_section(sect, addr);
        m_regs.regs[inst.i.rt] = *reinterpret_cast<int32_t*>(sect->sect.data() + offset);
    }
    break;
    case uint32_t(instructions::LB):
    {
        uint32_t addr = m_regs.regs[inst.i.rs] + bit_cast<int16_t>(inst.i.imm);

        section* sect = nullptr;
        if (!(sect = get_section_for_address(addr)) || !is_safe_access(sect, addr, sizeof(int8_t))) {
            throw mips_exception_load("Invalid memory access for LB operation", addr);
        }

        uint32_t offset = get_offset_for_section(sect, addr);
        m_regs.regs[inst.i.rt] = *reinterpret_cast<int8_t*>(sect->sect.data() + offset); // sign extend
    }
    break;
    case uint32_t(instructions::LH):
    {
        uint32_t addr = m_regs.regs[inst.i.rs] + bit_cast<int16_t>(inst.i.imm);

        section* sect = nullptr;
        if (!(sect = get_section_for_address(addr)) || !is_safe_access(sect, addr, sizeof(int16_t))) {
            throw mips_exception_load("Invalid memory access for LH operation", addr);
        }

        uint32_t offset = get_offset_for_section(sect, addr);
        m_regs.regs[inst.i.rt] = *reinterpret_cast<int16_t*>(sect->sect.data() + offset); // sign extend
    }
    break;
    case uint32_t(instructions::LBU):
    {
        uint32_t addr = m_regs.regs[inst.i.rs] + bit_cast<int16_t>(inst.i.imm);

        section* sect = nullptr;
        if (!(sect = get_section_for_address(addr)) || !is_safe_access(sect, addr, sizeof(uint8_t))) {
            throw mips_exception_load("Invalid memory access for LBU operation", addr);
        }

        uint32_t offset = get_offset_for_section(sect, addr);
        m_regs.regs[inst.i.rt] = *reinterpret_cast<uint8_t*>(sect->sect.data() + offset); // zero extend
    }
    break;
    case uint32_t(instructions::LHU):
    {
        uint32_t addr = m_regs.regs[inst.i.rs] + bit_cast<int16_t>(inst.i.imm);

        section* sect = nullptr;
        if (!(sect = get_section_for_address(addr)) || !is_safe_access(sect, addr, sizeof(uint16_t))) {
            throw mips_exception_load("Invalid memory access for LHU operation", addr);
        }

        uint32_t offset = get_offset_for_section(sect, addr);
        m_regs.regs[inst.i.rt] = *reinterpret_cast<uint16_t*>(sect->sect.data() + offset);  // zero extend
    }
    break;
    case uint32_t(instructions::SW):
    {
        uint32_t addr = m_regs.regs[inst.i.rs] + bit_cast<int16_t>(inst.i.imm);

        section* sect = nullptr;
        if (!(sect = get_section_for_address(addr)) || !(sect->flags & MUTABLE) || !is_safe_access(sect, addr, sizeof(uint32_t))) {
            throw mips_exception_store("Invalid memory access for SW operation", addr);
        }

        uint32_t offset = get_offset_for_section(sect, addr);
        *reinterpret_cast<uint32_t*>(sect->sect.data() + offset) = m_regs.regs[inst.i.rt];
    }
    break;
    case uint32_t(instructions::SB):
    {
        uint32_t addr = m_regs.regs[inst.i.rs] + bit_cast<int16_t>(inst.i.imm);

        section* sect = nullptr;
        if (!(sect = get_section_for_address(addr)) || !(sect->flags & MUTABLE) || !is_safe_access(sect, addr, sizeof(uint8_t))) {
            throw mips_exception_store("Invalid memory access for SB operation", addr);
        }

        uint32_t offset = get_offset_for_section(sect, addr);
        *reinterpret_cast<uint8_t*>(sect->sect.data() + offset) = m_regs.regs[inst.i.rt];
    }
    break;
    case uint32_t(instructions::SH):
    {
        uint32_t addr = m_regs.regs[inst.i.rs] + bit_cast<int16_t>(inst.i.imm);

        section* sect = nullptr;
        if (!(sect = get_section_for_address(addr)) || !(sect->flags & MUTABLE) || !is_safe_access(sect, addr, sizeof(uint16_t))) {
            throw mips_exception_store("Invalid memory access for SH operation", addr);
        }

        uint32_t offset = get_offset_for_section(sect, addr);
        *reinterpret_cast<uint16_t*>(sect->sect.data() + offset) = m_regs.regs[inst.i.rt];
    }
    break;
    default:
        throw std::runtime_error("Invalid instruction opcode");
    }

    return true;
}

bool executor::dispatch_funct(instruction inst) {
    switch (inst.r.funct) {
    case uint32_t(funct::SYSCALL):
    {
        return dispatch_syscall();
    }
    break;
    case uint32_t(funct::BREAK):
    {
        throw mips_exception_breakpoint("Breakpoint encountered");
    }
    break;
    case uint32_t(funct::SLL):
    {
        m_regs.regs[inst.r.rd] = m_regs.regs[inst.r.rt] << inst.r.shift;
    }
    break;
    case uint32_t(funct::SRL):
    {
        m_regs.regs[inst.r.rd] = m_regs.regs[inst.r.rt] >> inst.r.shift;
    }
    break;
    case uint32_t(funct::SLT):
    {
        m_regs.regs[inst.r.rd] = (int32_t(m_regs.regs[inst.r.rs]) < int32_t(m_regs.regs[inst.r.rt]));
    }
    break;
    case uint32_t(funct::SLTU):
    {
        m_regs.regs[inst.r.rd] = (m_regs.regs[inst.r.rs] < m_regs.regs[inst.r.rt]);
    }
    break;
    case uint32_t(funct::SRA):
    {
        m_regs.regs[inst.r.rd] = int32_t(m_regs.regs[inst.r.rt]) >> inst.r.shift;
    }
    break;
    case uint32_t(funct::JR):
    {
        m_regs.pc = m_regs.regs[inst.r.rs];

        return false; // dont advance pc
    }
    break;
    case uint32_t(funct::JALR):
    {
        m_regs.regs[int(register_names::ra)] = m_regs.pc + 0x4;
        m_regs.pc = m_regs.regs[inst.r.rs];

        return false; // dont advance pc
    }
    break;
    case uint32_t(funct::MFHI):
    {
        m_regs.regs[inst.r.rd] = m_regs.hi;
    }
    break;
    case uint32_t(funct::MTHI):
    {
        m_regs.hi = m_regs.regs[inst.r.rs];
    }
    break;
    case uint32_t(funct::MFLO):
    {
        m_regs.regs[inst.r.rd] = m_regs.lo;
    }
    break;
    case uint32_t(funct::MTLO):
    {
        m_regs.lo = m_regs.regs[inst.r.rs];
    }
    break;
    case uint32_t(funct::DIV):
    {
        int32_t a = m_regs.regs[inst.r.rs];
        int32_t b = m_regs.regs[inst.r.rt];

        if (m_regs.regs[inst.r.rt] == 0) {
            throw mips_exception_zero_division("Attempted division by 0");
        }

        m_regs.hi = a % b;
        m_regs.lo = a / b;
    }
    break;
    case uint32_t(funct::DIVU):
    {
        uint32_t a = m_regs.regs[inst.r.rs];
        uint32_t b = m_regs.regs[inst.r.rt];

        if (m_regs.regs[inst.r.rt] == 0) {
            throw mips_exception_zero_division("Attempted division by 0");
        }

        m_regs.hi = a % b;
        m_regs.lo = a / b;
    }
    break;
    case uint32_t(funct::MULT):
    {
        int64_t res = int64_t(m_regs.regs[inst.r.rs]) * int64_t(m_regs.regs[inst.r.rt]);
        m_regs.hi = res >> 32;
        m_regs.lo = res & 0xFFFFFFFF;
    }
    break;
    case uint32_t(funct::MULTU):
    {
        uint64_t res = uint64_t(m_regs.regs[inst.r.rs]) * uint64_t(m_regs.regs[inst.r.rt]);
        m_regs.hi = res >> 32;
        m_regs.lo = res & 0xFFFFFFFF;
    }
    break;
    case uint32_t(funct::ADD):
    {
        int32_t a = m_regs.regs[inst.r.rs];
        int32_t b = m_regs.regs[inst.r.rt];
        // check for overflow
        if ((b > 0 && a > std::numeric_limits<int32_t>::max() - b) || (b < 0 && a < std::numeric_limits<int32_t>::min() - b)) {
            throw mips_exception_arithmetic_overflow("ADD operation overflowed");
        }

        m_regs.regs[inst.r.rd] = a + b;
    }
    break;
    case uint32_t(funct::ADDU):
    {
        m_regs.regs[inst.r.rd] = m_regs.regs[inst.r.rs] + m_regs.regs[inst.r.rt];
    }
    break;
    case uint32_t(funct::SUB):
    {
        int32_t a = m_regs.regs[inst.r.rs];
        int32_t b = m_regs.regs[inst.r.rt];
        // check for overflow
        if ((b < 0 && a > std::numeric_limits<int32_t>::max() + b) || (b > 0 && a < std::numeric_limits<int32_t>::min() + b)) {
            throw mips_exception_arithmetic_overflow("SUB operation overflowed");
        }

        m_regs.regs[inst.r.rd] = a - b;
    }
    break;
    case uint32_t(funct::SUBU):
    {
        m_regs.regs[inst.r.rd] = m_regs.regs[inst.r.rs] - m_regs.regs[inst.r.rt];
    }
    break;
    case uint32_t(funct::AND):
    {
        m_regs.regs[inst.r.rd] = m_regs.regs[inst.r.rs] & m_regs.regs[inst.r.rt];
    }
    break;
    case uint32_t(funct::OR):
    {
        m_regs.regs[inst.r.rd] = m_regs.regs[inst.r.rs] | m_regs.regs[inst.r.rt];
    }
    break;
    case uint32_t(funct::XOR):
    {
        m_regs.regs[inst.r.rd] = m_regs.regs[inst.r.rs] ^ m_regs.regs[inst.r.rt];
    }
    break;
    case uint32_t(funct::NOR):
    {
        m_regs.regs[inst.r.rd] = ~(m_regs.regs[inst.r.rs] | m_regs.regs[inst.r.rt]);
    }
    break;
    case uint32_t(funct::TGE):
    {
        if (int32_t(m_regs.regs[inst.r.rs]) >= int32_t(m_regs.regs[inst.r.rt])) {
            throw mips_exception_trap("Trap exception");
        }
    }
    break;
    case uint32_t(funct::TGEU):
    {
        if (m_regs.regs[inst.r.rs] >= m_regs.regs[inst.r.rt]) {
            throw mips_exception_trap("Trap exception");
        }
    }
    break;
    case uint32_t(funct::TLT):
    {
        if (int32_t(m_regs.regs[inst.r.rs]) < int32_t(m_regs.regs[inst.r.rt])) {
            throw mips_exception_trap("Trap exception");
        }
    }
    break;
    case uint32_t(funct::TLTU):
    {
        if (m_regs.regs[inst.r.rs] < m_regs.regs[inst.r.rt]) {
            throw mips_exception_trap("Trap exception");
        }
    }
    break;
    case uint32_t(funct::TEQ):
    {
        if (m_regs.regs[inst.r.rs] == m_regs.regs[inst.r.rt]) {
            throw mips_exception_trap("Trap exception");
        }
    }
    break;
    case uint32_t(funct::TNE):
    {
        if (m_regs.regs[inst.r.rs] != m_regs.regs[inst.r.rt]) {
            throw mips_exception_trap("Trap exception");
        }
    }
    break;
    default:
        throw std::runtime_error("Invalid funct number");
    }

    return true;
}


bool executor::dispatch_syscall() {
    uint32_t syscall_num = m_regs.regs[int(register_names::v0)];
    uint32_t a0 = m_regs.regs[int(register_names::a0)];
    uint32_t a1 = m_regs.regs[int(register_names::a1)];
    uint32_t a2 = m_regs.regs[int(register_names::a2)];

    switch (syscall_num) {
    case uint32_t(syscalls::PRINT_INT):
    {
        printf("%i", a0);
    }
    break;
    case uint32_t(syscalls::PRINT_FLOAT):
    {
        printf("%f", m_regs.f[12]);
    }
    break;
    case uint32_t(syscalls::PRINT_DBL):
    {
        printf("%f", m_regs.f[12]);
    }
    break;
    case uint32_t(syscalls::PRINT_STRING):
    {
        section* sect = nullptr;
        if (!(sect = get_section_for_address(a0))) {
            throw mips_exception_load("Invalid memory access for PRINT_STRING syscall", a0);
        }
        uint32_t offset = get_offset_for_section(sect, a0);
        const char* str = (const char*)(sect->sect.data() + offset);

        // make sure string actually terminates so we don't crash or leak memory
        if (!string_terminates(str, sect->sect.size() - offset)) {
            throw mips_exception_load("Invalid string for PRINT_STRING syscall, does not terminate", a0);
        }

        printf("%s", str);
    }
    break;
    case uint32_t(syscalls::READ_INT):
    {
        disable_conio_mode();
        int32_t in;
        std::cin >> in;
        m_regs.regs[int(register_names::v0)] = in;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    break;
    case uint32_t(syscalls::READ_FLOAT):
    {
        disable_conio_mode();
        float in;
        std::cin >> in;
        m_regs.f[0] = in;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    break;
    case uint32_t(syscalls::READ_DBL):
    {
        disable_conio_mode();
        float in;
        std::cin >> in;
        m_regs.f[0] = in;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    break;
    case uint32_t(syscalls::READ_STRING):
    {
        section* sect = nullptr;
        if (!(sect = get_section_for_address(a0)) || !(sect->flags & MUTABLE) || !is_safe_access(sect, a0, a1)) {
            throw mips_exception_store("Invalid memory access for READ_STRING syscall", a0);
        }

        disable_conio_mode();

        std::string in;
        std::getline(std::cin, in);

        // truncate string to correct length if needed 
        if (in.length() > a1 - 1) {
            in.resize(a1 - 1);
        }
        // if space exists, add newline
        if (in.length() < a1 - 1) {
            in.append("\n");
        }

        uint32_t offset = get_offset_for_section(sect, a0);
        memcpy(sect->sect.data() + offset, in.c_str(), in.length() + 1);
    }
    break;
    case uint32_t(syscalls::SBRK):
    {
        m_regs.regs[int(register_names::v0)] = m_heap.sbrk(a0);
    }
    break;
    case uint32_t(syscalls::EXIT):
    {
        throw mips_exception_exit();
    }
    break;
    case uint32_t(syscalls::PRINT_CHAR):
    {
        printf("%c", a0);
    }
    break;
    case uint32_t(syscalls::READ_CHAR):
    {
        disable_conio_mode();
        m_regs.regs[int(register_names::v0)] = getchar();
    }
    break;
    case uint32_t(syscalls::OPEN_FILE):
    {
        section* sect = nullptr;
        if (!(sect = get_section_for_address(a0))) {
            throw mips_exception_load("Invalid memory access for OPEN_FILE syscall", a0);
        }
        uint32_t offset = get_offset_for_section(sect, a0);
        const char* filename = (const char*)(sect->sect.data() + offset);

        // make sure string actually terminates so we don't crash or leak memory
        if (!string_terminates(filename, sect->sect.size() - offset)) {
            throw mips_exception_load("Invalid string for OPEN_FILE syscall, does not terminate", a0);
        }

        m_regs.regs[int(register_names::v0)] = m_file_mgr.open_file(filename, a1, a2);

    }
    break;
    case uint32_t(syscalls::READ_FILE):
    {
        section* sect = nullptr;
        if (!(sect = get_section_for_address(a1)) || !(sect->flags & MUTABLE) || !is_safe_access(sect, a1, a2)) {
            throw mips_exception_store("Invalid memory access for READ_FILE syscall", a1);
        }

        uint32_t offset = get_offset_for_section(sect, a1);
        m_regs.regs[int(register_names::v0)] = m_file_mgr.read_file(a0, sect->sect.data() + offset, a2);
    }
    break;
    case uint32_t(syscalls::WRITE_FILE):
    {
        section* sect = nullptr;
        if (!(sect = get_section_for_address(a1)) || !is_safe_access(sect, a1, a2)) {
            throw mips_exception_store("Invalid memory access for READ_FILE syscall", a1);
        }

        uint32_t offset = get_offset_for_section(sect, a1);
        m_regs.regs[int(register_names::v0)] = m_file_mgr.write_file(a0, sect->sect.data() + offset, a2);
    }
    break;
    case uint32_t(syscalls::CLOSE_FILE):
    {
        m_file_mgr.close_file(a0);
    }
    break;
    case uint32_t(syscalls::EXIT2):
    {
        throw mips_exception_exit("EXIT syscall invoked, terminating with value " + std::to_string(a0));
    }
    break;
    case uint32_t(syscalls::TIME):
    {
        // get time since epoch
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        m_regs.regs[int(register_names::a0)] = (uint32_t)(timestamp & 0xFFFFFFFF);
        m_regs.regs[int(register_names::a1)] = (uint32_t)((timestamp >> 32) & 0xFFFFFFFF);
    }
    break;
    case uint32_t(syscalls::SLEEP):
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(a0));
    }
    break;
    case uint32_t(syscalls::PRINT_HEX):
    {
        printf("%X", a0);
    }
    break;
    case uint32_t(syscalls::PRINT_BINARY):
    {
        std::stringstream ss;
        ss << std::bitset<32>(a0);
        printf("%s", ss.str().c_str());
    }
    break;
    case uint32_t(syscalls::PRINT_UNSIGNED):
    {
        printf("%u", a0);
    }
    break;
    case uint32_t(syscalls::SET_SEED):
    {
        m_random_mgr.set_seed(a0, a1);
    }
    break;
    case uint32_t(syscalls::RAND_INT):
    {
        m_regs.regs[int(register_names::a0)] = m_random_mgr.get_int(a0);
    }
    break;
    case uint32_t(syscalls::RAND_INT_RANGE):
    {
        m_regs.regs[int(register_names::a0)] = m_random_mgr.get_int_range(a0, a1);
    }
    break;
    case uint32_t(syscalls::RAND_FLOAT):
    {
        m_regs.f[0] = m_random_mgr.get_float(a0);
    }
    break;
    case uint32_t(syscalls::RAND_DBL):
    {
        m_regs.f[0] = m_random_mgr.get_float(a0);
    }
    break;
    default:
        throw mips_exception_syscall("Syscall number " + std::to_string(syscall_num) + " not implemented");

    }

    return true;
}