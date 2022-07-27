#include "pch.h"
#include "executor.h"
#include "helper.h"


int main(int argc, char** argv) {
    //int32_t i = 0x714b4802;
    //int32_t i2 = 0x012a0018;
    //instruction inst(i);
    //instruction inst2(i2);
    
    //printf("%X | %i | %i | %i | %i\n", inst.r.opcode, inst.r.rs, inst.r.rt, inst.r.rd, inst.r.funct);
    //printf("%X | %i | %i | %i | %i\n", inst2.r.opcode, inst2.r.rs, inst2.r.rt, inst2.r.rd, inst2.r.funct);

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
