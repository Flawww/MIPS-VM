#include "pch.h"
#include "executor.h"
#include "helper.h"
#include "file_mgr.h"

executor::executor(std::string file): m_can_run(false), m_tick(0), m_kernelmode(false), m_has_exception_handler(false) {
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
            printf("Size of binary file '%s%s' too small or unaligned (%X bytes)\n", file.c_str(), section_names[i], uint32_t(m_sections[i].sect.size()) - 4);
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

    // check if exception handler exists
    m_kernelmode = true; // need to set the kernelmode flag to true to be able to get the kernelmode section
    section* ktext = get_section_for_address(EXCEPTION_HANDLER);
    if (ktext && ktext->address == m_sections[KTEXT].address) {
        m_has_exception_handler = true; // address 0x80000180 (exception handler) is valid and in .ktext, exception handler exists.
    }
    m_kernelmode = false;

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
    //printf("true\n");
    return true;
}

void executor::run() {
    for (int i = 0; i < NUM_SECTIONS; i++) {
        printf("%s @ 0x%08X, length %X\n", section_names[i], m_sections[i].address, uint32_t(m_sections[i].sect.size()));
    }

    printf("\nExecuting bytecode...\n\n===========================================\n");

    std::string exit_reason;
    instruction inst(0x0);

    while (true) {
        bool error_state = false;
        std::exception err;

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
                m_regs.pc += 0x4; // next instruction - if dispatch returns false dont increase pc (eg. jump/ret instructions)
            }
            m_regs.regs[0] = 0; // in case if someone wrote to $zero, make sure to reset it immediately

            // check keyboard interrupt(s)
            keyboard_interrupt();

            // check if we reached end of .text 
            if (m_regs.pc == m_sections[TEXT].address + m_sections[TEXT].sect.size() || (m_sections[KTEXT].address && m_regs.pc == m_sections[KTEXT].address + m_sections[KTEXT].sect.size())) {
                exit_reason = "dropped off bottom";
                break; // exit graccefully
            }
        }
        catch (const mips_exception_exit& e) { // EXIT syscall
            exit_reason = std::string(e.what());
            break;
        }
        catch (const mips_exception& e) { // generic exception that a exception handler could handle
            if (m_has_exception_handler && !m_kernelmode) {
                if (e.invalid_memory_address()) {
                    m_regs.vaddr = e.get_vaddr(); // set vaddr to invalid address if the exception was an invalid memory address
                }

                m_regs.status = (1 << 1); // bit 1 is set
                m_regs.cause = e.exception_type() << 2; // bits 2-6 of cause is exception type. bit 8 is pending interrupt. Shift left by 2 to make it the correct bits.
                m_regs.epc = m_regs.pc; // save pc of instruction which caused exception

                m_kernelmode = true; // enter kernelmode
                m_regs.pc = EXCEPTION_HANDLER;

                
            }
            else {
                err = e;
                error_state = true;
            }
        }
        catch (const std::exception& e) {
            err = e;
            error_state = true;          
        }

        if (error_state) {
            printf("Error: %s\n", err.what());
            printf("Error on instruction %02X (0x%08X) with PC: 0x%08X\n", inst.r.opcode, inst.hex, m_regs.pc);
            exit_reason = "error occured during execution";
            break;
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

    // Use Mars' default value of 5 tick update interval (the keyboard interrupt data will only update at most once every 5 ticks)
    // also, we do not want to throw an exception while we are already in kernelmode
    if (m_tick % 5 != 0 || m_kernelmode) {
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