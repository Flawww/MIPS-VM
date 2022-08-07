#include "pch.h"
#include "executor.h"
#include "helper.h"


int main(int argc, char** argv) {
    //int32_t i = 0x012a0036;
    //int32_t i2 = 0x052e0064;
    //instruction inst(i);
    //instruction inst2(i2);
    //
    //printf("0x%02X | %i | %i | %i | %i\n", inst.r.opcode, inst.r.rs, inst.r.rt, inst.r.rd, inst.r.funct);
    //printf("0x%02X | %i | %i | %i\n", inst2.i.opcode, inst2.i.rs, inst2.i.rt, inst2.i.imm);
    //printf("0x%02X | %i\n", inst.j.opcode, inst.j.p_addr);
    //printf("%X | %i | %i | %i | %i\n", inst2.r.opcode, inst2.r.rs, inst2.r.rt, inst2.r.rd, inst2.r.funct);

    std::string program;
    if (argc < 2) {
        printf("Enter name of the program: ");
        std::getline(std::cin, program);
    }
    else {
        program = std::string(argv[1]);
    }
    
    // set up signal handling for conio on Linux
    setup_signal_interceptor();


    // start the vm with the specified input file
    executor vm(program);
    if (!vm.can_run()) {
        printf("Error: MIPS Virtual Machine could not be initialized\n");
        disable_conio_mode();
        return 1;
    }

    vm.run();

    disable_conio_mode();
    std::getchar();
    return 0;
} 
