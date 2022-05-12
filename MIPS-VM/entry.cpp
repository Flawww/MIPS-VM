#include "pch.h"
#include "executor.h"


int main(int argc, char** argv) {
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
