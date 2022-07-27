#include "pch.h"
#include "executor.h"
#include "helper.h"

executor::executor(std::string file) {
    m_can_run = false;

    // load text section
    std::ifstream text(file + ".text", std::ios::binary);
    if (!text.is_open()) {
        return;
    }

    m_text.sect = std::vector<uint8_t>(std::istreambuf_iterator<char>(text), {});
    m_text.address = 0x00400000;
    // ensure divisible by 4, MIPS instruction set is always 4 bytes
    if ((m_text.sect.size() % 4) != 0) {
        printf("Size of bytecode not evenly divisible by 4, invalid.\n");
        return;
    }

    // load ktext section if one exists
    std::ifstream ktext(file + ".ktext", std::ios::binary);
    if (ktext.is_open()) {
        m_ktext.sect = std::vector<uint8_t>(std::istreambuf_iterator<char>(ktext), {});
        m_ktext.address = 0x80000000;
    }

    // load data section if one exists
    std::ifstream data(file + ".data", std::ios::binary);
    if (data.is_open()) {
        m_data.sect = std::vector<uint8_t>(std::istreambuf_iterator<char>(data), {});
        m_data.address = 0x10010000;
    }
    
    m_regs.regs[int(register_names::sp)] = 0x7FFFEFFC;
    m_regs.pc = m_text.address;
    m_can_run = true;
}

uint32_t executor::get_offset_for_section(section* sect, uint32_t addr) {
    return addr - sect->address;
}

section* executor::get_section_for_address(uint32_t addr) {
    if (addr >= m_data.address && addr < m_data.address + m_data.sect.size()) {
        return &m_data;
    }
    else if (addr >= m_text.address && addr < m_text.address + m_text.sect.size()) {
        return &m_text;
    }
    else if (addr >= m_ktext.address && addr < m_ktext.address + m_ktext.sect.size()) {
        return &m_ktext;
    }
    else if (m_heap.get_section_if_valid_heap(addr)) {
        return m_heap.get_section_if_valid_heap(addr);
    }
    else if (m_stack.get_section_if_valid_stack(addr)) {
        return m_stack.get_section_if_valid_stack(addr);
    }

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
    printf(".text length: %i\n.data length: %i\n", m_text.sect.size(), m_data.sect.size());
    printf(".text\n");
    for (int i = 0; i < m_text.sect.size() / 0x4; i++) {
        printf("%08x\n", *reinterpret_cast<uint32_t*>(m_text.sect.data() + i * 0x4));
    }

    printf(".data\n");
    for (int i = 0; i < 25; i++) {
        printf("%c", m_data.sect[i]);
    }
    printf("\n");
    printf("Test print: %s\n", m_data.sect.data());

    printf("pc: %08X\n", m_regs.pc);

    bool increment_pc = true;
    while (true) {
        increment_pc = true;

        auto section = get_section_for_address(m_regs.pc);
        if (!section) { // trying to execute invalid memory - end of program or error
            break;
        }

        uint32_t offset = m_regs.pc - section->address;
        // read next instruction to execute 
        instruction inst = instruction(*reinterpret_cast<uint32_t*>(section->sect.data() + offset));

        // dispatch instruction now
        try {
            if (!dispatch(inst)) {
                increment_pc = false;
            }
        }
        catch (const std::runtime_error& e) {
            printf("Error: %s", e.what());
            break;
        }

        printf("%02X ", inst.i.opcode);
        if (increment_pc) {
            m_regs.pc += 0x4; // next instruction
        }  
    }
}

bool executor::dispatch(instruction inst) {
    // get the opcode (opcode is always the first 6 bits, so what format we get the opcode as does not matter)
    switch (inst.r.opcode) {
    case uint32_t(instructions::R_FORMAT):
    {
        return dispatch_funct(inst);
    }
    break;
    case uint32_t(instructions::MFC0):
    {
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
    case uint32_t(instructions::J):
    {
        uint32_t addr = inst.j.p_addr * 4;
        addr |= (m_regs.pc + 4) & 0xF0000000;
        m_regs.pc = addr;

        return false; // dont advance pc
    }
    break;
    case uint32_t(instructions::JAL):
    {
        m_regs.regs[int(register_names::ra)] = m_regs.pc + 0x4;

        uint32_t addr = inst.j.p_addr * 4;
        addr |= (m_regs.pc + 4) & 0xF0000000;
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
            m_regs.pc += bit_cast<int16_t>(inst.i.imm) * 4;

            return false; // dont advance pc
        }
    }
    break;
    case uint32_t(instructions::BNE):
    {
        if (m_regs.regs[inst.i.rs] != m_regs.regs[inst.i.rt]) {
            m_regs.pc += bit_cast<int16_t>(inst.i.imm) * 4;

            return false; // dont advance pc
        }
    }
    break;
    case uint32_t(instructions::BLEZ):
    {
        if (m_regs.regs[inst.i.rs] <= 0) {
            m_regs.pc += bit_cast<int16_t>(inst.i.imm) * 4;

            return false; // dont advance pc
        }
    }
    break;
    case uint32_t(instructions::BGTZ):
    {
        if (m_regs.regs[inst.i.rs] > 0) {
            m_regs.pc += bit_cast<int16_t>(inst.i.imm) * 4;

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
        if (!(sect = get_section_for_address(addr)) || !is_safe_access(sect, addr, sizeof(uint32_t))) {
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
        if (!(sect = get_section_for_address(addr)) || !is_safe_access(sect, addr, sizeof(uint8_t))) {
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
        if (!(sect = get_section_for_address(addr)) || !is_safe_access(sect, addr, sizeof(uint16_t))) {
            throw std::runtime_error("Invalid memory access for SB operation");
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
    case uint32_t(funct::SLL):
    {

    }
    break;
    case uint32_t(funct::DIV):
    {

    }
    break;
    case uint32_t(funct::DIVU):
    {

    }
    break;
    case uint32_t(funct::SRL):
    {

    }
    break;
    case uint32_t(funct::SLT):
    {

    }
    break;
    case uint32_t(funct::SLTU):
    {

    }
    break;
    case uint32_t(funct::SRA):
    {

    }
    break;
    case uint32_t(funct::JR):
    {

    }
    break;
    case uint32_t(funct::JALR):
    {

    }
    break;
    case uint32_t(funct::MFHI):
    {

    }
    break;
    case uint32_t(funct::MTHI):
    {

    }
    break;
    case uint32_t(funct::MFLO):
    {

    }
    break;
    case uint32_t(funct::MTLO):
    {

    }
    break;
    case uint32_t(funct::MULT):
    {

    }
    break;
    case uint32_t(funct::MULTU):
    {

    }
    break;
    case uint32_t(funct::ADD):
    {

    }
    break;
    case uint32_t(funct::ADDU):
    {

    }
    break;
    case uint32_t(funct::SUB):
    {

    }
    break;
    case uint32_t(funct::SUBU):
    {

    }
    break;
    case uint32_t(funct::AND):
    {

    }
    break;
    case uint32_t(funct::OR):
    {

    }
    break;
    case uint32_t(funct::XOR):
    {

    }
    break;
    case uint32_t(funct::NOR):
    {

    }
    break;
    default:
        return false;
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
        printf("%s", (const char*)a0);
    }
    break;
    case uint32_t(syscalls::READ_INT):
    {
        int32_t in;
        std::cin >> in;
        m_regs.regs[int(register_names::v0)] = in;
    }
    break;
    case uint32_t(syscalls::READ_FLOAT):
    {
        float in;
        std::cin >> in;
        m_regs.f[0] = in;
    }
    break;
    case uint32_t(syscalls::READ_DBL):
    {
        float in;
        std::cin >> in;
        m_regs.f[0] = in;
    }
    break;
    case uint32_t(syscalls::READ_STRING):
    {
        section* sect = get_section_for_address(a0);
        if (!sect) {
            throw std::runtime_error("Invalid address passed to READ_STRING syscall");
        }
        if (!is_safe_access(sect, a0, a1)) {
            throw std::runtime_error("Invalid memory access for READ_STRING syscall");
        }

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
        throw std::runtime_error("Exit syscalled called");
    }
    break;
    case uint32_t(syscalls::PRINT_CHAR):
    {
        printf("%c", a0);
    }
    break;
    case uint32_t(syscalls::READ_CHAR):
    {
        m_regs.regs[int(register_names::v0)] = getchar();
    }
    break;
    default:
        throw std::runtime_error("Syscall number " + std::to_string(syscall_num) + " not implemented");
        
    }

    return true;
}