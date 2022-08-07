#include "pch.h"
#include "executor.h"
#include "helper.h"

executor::executor(std::string file): m_can_run(false), m_tick(0), m_kernelmode(false) {

    // load all existing sections
    for (int i = 0; i < NUM_SECTIONS; i++) {
        std::ifstream bin(file + section_names[i], std::ios::binary);
        if (!bin.is_open()) {
            continue;
        }

        // read the binary file into a buffer
        m_sections[i].sect = std::vector<uint8_t>(std::istreambuf_iterator<char>(bin), {});
        m_sections[i].flags = section_protection[i]; // get the protection flags for this section

        // make sure executable sections are aligned to 4 bytes and a nonzero size (4 first bytes of the buffer is used for the section address)
        if ((m_sections[i].flags & EXECUTABLE && m_sections[i].sect.size() & 0x3) || m_sections[i].sect.size() - 4 <= 0) {
            printf("Size of binary file '%s%s' too small or unaligned (%X bytes)\n", file.c_str(), section_names[i], m_sections[i].sect.size() - 4);
            m_sections[i] = section();
            continue;
        }

        // read the first 4 bytes of the loaded file, it denotes the address of the section
        m_sections[i].address = *reinterpret_cast<uint32_t*>(m_sections[i].sect.data());
        // remove the 4 byte section address at the start of the buffer
        m_sections[i].sect.erase(m_sections[i].sect.begin(), m_sections[i].sect.begin() + 4);
    }

    if (!m_sections[TEXT].address) {
        printf(".text section for program %s not loaded - aborting\n", file.c_str());
        return;
    }

    // create MMIO section
    m_mmio.sect.resize(2 * sizeof(uint32_t), 0); // 8 bytes
    m_mmio.flags = MUTABLE;
    m_mmio.address = 0xFFFF0000;
    
    m_regs.regs[int(register_names::sp)] = 0x7FFFEFFC;
    m_regs.pc = m_sections[TEXT].address;

    m_can_run = true;
}

uint32_t executor::get_offset_for_section(section* sect, uint32_t addr) {
    return addr - sect->address;
}

section* executor::get_section_for_address(uint32_t addr) {
    if (addr == 0) {
        return nullptr;
    }

    // check if its in a section from the "executable" (text, data, ktext or kdata)
    for (int i = 0; i < NUM_SECTIONS; i++) {
        // skip kernelmode address space if we are not in kernelmode
        if ((i == KTEXT || i == KDATA) && !m_kernelmode) {
            continue;
        }
        if (addr >= m_sections[i].address && addr < m_sections[i].address + m_sections[i].sect.size()) {
            return &m_sections[i];
        }
    }

    // check heap & stack
    if (m_heap.get_section_if_valid_heap(addr)) {
        return m_heap.get_section_if_valid_heap(addr);
    }
    else if (m_stack.get_section_if_valid_stack(addr)) {
        return m_stack.get_section_if_valid_stack(addr);
    }

    // finally check MMIO
    if (addr >= m_mmio.address && addr < m_mmio.address + m_mmio.sect.size()) {
        return &m_mmio;
    }

    // Invalid address, return null
    return nullptr;
}

bool executor::is_safe_access(section* sect, uint32_t addr, uint32_t size) {
    uint32_t sect_start = sect->address;
    uint32_t sect_end = sect->address + sect->sect.size();
    if (addr < sect_start || addr >= sect_end ||
        addr + size < sect_start || addr + size > sect_end) {
        return false;
    }

    return true;
}

void executor::run() {
    for (int i = 0; i < NUM_SECTIONS; i++) {
        printf(".%s @ %08X, length %X\n", section_names[i], m_sections[i].address, m_sections[i].sect.size());
    }

    printf("\nExecuting bytecode...\n\n===========================================\n");

    std::string exit_reason;
    instruction inst(0x0);
    while (true) {
        try {
            section* section = get_section_for_address(m_regs.pc);
            if (!section || !(section->flags & EXECUTABLE) || (m_regs.pc & 0x3)) { // trying to execute invalid memory (Invalid address, not an executable section or address not 4-aligned)
                throw std::runtime_error("Invalid PC, tried executing invalid, protected or non-aligned memory");
            }
            // get_section_for_address will not return a kernelmode address if we are currently in usermode, but we don't want to execute usermode .text from kernelmode either
            if (m_kernelmode && section->address == m_sections[TEXT].address) {
                throw std::runtime_error("Tried executing usermode memory from kernelmode");
            }

            uint32_t offset = get_offset_for_section(section, m_regs.pc);
            // read next instruction to execute 
            inst = instruction(*reinterpret_cast<uint32_t*>(section->sect.data() + offset));

            // dispatch instruction now
            if (dispatch(inst)) {
                m_regs.pc += 0x4; // next instruction - if dispatch returns false it means its a jump instruction, dont increase pc
            }
   
            m_regs.regs[0] = 0; // in case if someone wrote to $zero, make sure to reset it

            // check keyboard interrupt(s)
            keyboard_interrupt();

            // check if we reached end of .text 
            if (m_regs.pc == m_sections[TEXT].address + m_sections[TEXT].sect.size() || (m_sections[KTEXT].address && m_regs.pc == m_sections[KTEXT].address + m_sections[KTEXT].sect.size())) {
                exit_reason = "dropped off bottom";
                break; // exit graccefully
            }
        }
        catch (const mips_exception_exit& e) {
            exit_reason = std::string(e.what());
            break;
        }
        catch (const std::runtime_error& e) {
            printf("Error: %s\n", e.what());
            printf("Error on instruction %02X (0x%08X) with PC: 0x%08X\n", inst.r.opcode, inst.hex, m_regs.pc);
            exit_reason = "error occured during execution";
            return;
        }
        m_tick++;
    }

    printf("\n===========================================\nFinished executing (%s)\n", exit_reason.c_str());
}

void executor::keyboard_interrupt() {
    bool controller = *reinterpret_cast<uint32_t*>(m_mmio.sect.data()) & 0x2; // check bit 1 for "Keyboard interrupt enable"
    if (!controller) {
        disable_conio_mode(); // if keyboard interrupts are disabled, disable conio mode (linux)
        return;
    }
    enable_conio_mode(); // enable conio mode (for linux) to be able to use getch

    // Use Mars' default value of 5 ticks (the keyboard interrupt data will only update at most once every 5 ticks)
    if (m_tick % 5 != 0) {
        return;
    }

    char c = getch_noblock(); // read a character from stdin stream
    if (c == EOF) {
        return; // no character to read 
    }

    // write the character into mmio reciever data
    *reinterpret_cast<char*>(m_mmio.sect.data() + sizeof(uint32_t)) = c;

    // Throw interrupt exception
    throw mips_exception_interrupt("Keyboard interrupt");
}

bool executor::dispatch(instruction inst) {
    // get the opcode (opcode is always the first 6 bits, so what format we get the opcode as does not matter)
    switch (inst.r.opcode) {
    case uint32_t(instructions::R_FORMAT):
    {
        return dispatch_funct(inst);
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
        m_regs.regs[inst.i.rt] = int32_t(m_regs.regs[inst.i.rs]) + bit_cast<int16_t>(inst.i.imm);
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
            throw std::runtime_error("Invalid memory access for LW operation");
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
            throw std::runtime_error("Invalid memory access for LB operation");
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
            throw std::runtime_error("Invalid memory access for LH operation");
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
            throw std::runtime_error("Invalid memory access for LBU operation");
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
            throw std::runtime_error("Invalid memory access for LHU operation");
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
            throw std::runtime_error("Invalid memory access for SW operation");
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
            throw std::runtime_error("Invalid memory access for SB operation");
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
            throw std::runtime_error("Invalid memory access for SH operation");
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
        throw std::runtime_error("Breakpoint encountered");
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
            throw std::runtime_error("Attempted division by 0");
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
            throw std::runtime_error("Attempted division by 0");
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
            throw std::runtime_error("ADD operation overflowed: exception traps not yet implemented"); // TODO
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
            throw std::runtime_error("SUB operation overflowed: exception traps not yet implemented"); // TODO
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
    default:
        throw std::runtime_error("Invalid funct number for R-format");
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
            throw std::runtime_error("Invalid memory access for PRINT_STRING syscall");
        }
        uint32_t offset = get_offset_for_section(sect, a0);
        const char* str = (const char*)(sect->sect.data() + offset);

        // make sure string actually terminates so we don't crash or leak memory
        if (!string_terminates(str, sect->sect.size() - offset)) {
            throw std::runtime_error("Invalid string for PRINT_STRING syscall, does not terminate");
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
            throw std::runtime_error("Invalid memory access for READ_STRING syscall");
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
    case uint32_t(syscalls::EXIT2):
    {
        throw mips_exception_exit("EXIT syscall invoked, terminating with value " + std::to_string(a0));
    }
    break;
    case uint32_t(syscalls::SLEEP):
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(a0));
    }
    break;
    default:
        throw std::runtime_error("Syscall number " + std::to_string(syscall_num) + " not implemented");
        
    }

    return true;
}