#include "pch.h"
#include "executor.h"
#include "helper.h"


int main(int argc, char** argv) {
    int32_t i = 0x0c100009;
    int32_t i2 = 0x44891900;
    instruction inst(i);
    instruction inst2(i2);
    // 16 | 0 | 11 | 8 | 0
    //printf("%i | %i | %i | %i | %i\n", inst.r.opcode, inst.r.rs, inst.r.rt, inst.r.rd, inst.r.shift);
    //printf("%i | %i | %i | %i | %i\n", inst2.r.opcode, inst2.r.rs, inst2.r.rt, inst2.r.rd, inst2.r.shift);
    uint32_t addr = inst.j.p_addr * 4;
    addr |= (0x00400010 + 4) & 0xF0000000;
    printf("0x%08X | 0x%08X | 0x%08X\n", inst.j.p_addr, inst.j.p_addr * 4, addr);

    uint32_t a = 255;
    uint32_t b = *reinterpret_cast<int8_t*>(&a);
    printf("%u \n", 4 << 2);

    return 0;
    std::string program;
    if (argc < 2) {
        printf("Enter name of the program: ");
        std::getline(std::cin, program);
    }
    else {
        program = std::string(argv[1]);
    }
    
    // start the vm with the specified input file
    executor vm(program);
    if (!vm.can_run()) {
        printf("VM executor could not be initialized\n");
        return 1;
    }

    vm.run();

    std::getchar();
    return 0;
} 
