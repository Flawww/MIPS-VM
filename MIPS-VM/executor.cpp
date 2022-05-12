#include "pch.h"
#include "executor.h"

executor::executor(std::string file) {
    m_can_run = false;

    // load text section
    std::ifstream text(file + ".text", std::ios::binary);
    if (!text.is_open()) {
        return;
    }

    m_text.sect = std::vector<uint8_t>(std::istreambuf_iterator<char>(text), {});
    // ensure divisible by 4, MIPS instruction set is always 4 bytes
    if ((m_text.sect.size() % 4) != 0) {
        printf("Size of bytecode not evenly divisible by 4, invalid.\n");
        return;
    }

    // load ktext section if one exists
    std::ifstream ktext(file + ".ktext", std::ios::binary);
    if (ktext.is_open()) {
        m_ktext.sect = std::vector<uint8_t>(std::istreambuf_iterator<char>(ktext), {});
    }

    // load data section if one exists
    std::ifstream data(file + ".data", std::ios::binary);
    if (data.is_open()) {
        m_data.sect = std::vector<uint8_t>(std::istreambuf_iterator<char>(data), {});
    }

    m_can_run = true;
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

    return nullptr;
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
        bool ok = dispatch(inst);

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

    }
    break;
    case uint32_t(instructions::J):
    {

    }
    break;
    case uint32_t(instructions::JAL):
    {

    }
    break;
    case uint32_t(instructions::SLTI):
    {

    }
    break;
    case uint32_t(instructions::SLTIU):
    {

    }
    break;
    case uint32_t(instructions::ANDI):
    {

    }
    break;
    case uint32_t(instructions::ORI):
    {

    }
    break;
    case uint32_t(instructions::LUI):
    {

    }
    break;
    case uint32_t(instructions::SW):
    {

    }
    break;
    case uint32_t(instructions::BEQ):
    {

    }
    break;
    case uint32_t(instructions::BNE):
    {

    }
    break;
    case uint32_t(instructions::BLEZ):
    {

    }
    break;
    case uint32_t(instructions::BGTZ):
    {

    }
    break;
    case uint32_t(instructions::ADDI):
    {

    }
    break;
    case uint32_t(instructions::ADDIU):
    {

    }
    break;
    case uint32_t(instructions::LB):
    {

    }
    break;
    case uint32_t(instructions::LW):
    {

    }
    break;
    case uint32_t(instructions::LBU):
    {

    }
    break;
    case uint32_t(instructions::LHU):
    {

    }
    break;
    case uint32_t(instructions::SB):
    {

    }
    break;
    case uint32_t(instructions::SH):
    {

    }
    break;
    default:
        return false; // invalid opcode
    }

    return true;
}

bool executor::dispatch_funct(instruction inst) {
    switch (inst.r.funct) {
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