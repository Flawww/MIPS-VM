# MIPS Virtual Machine
Cross-platform virtual machine for MIPS, written in C++.

# Compilation
Requires a compiler that supports C++17 or later.
## Windows
Either:
* Open the project solution in Visual Studio and build normally through Visual Studio with MSVC.

or

* Run `build.bat` to build the project with GCC

## Linux
GCC needs to be installed. 
* Run `build.sh` to build the project. 

# Usage
After running the application, enter the name of the MIPS program name. The MIPS program at very least needs to have have a `.text` section. The files should be the (compiled) binary instructions for the program. Each section has a separate file, with different suffixes (`.text`,  `.data`,  `.ktext`,  `.kdata`).

To easily generate these binary files from MIPS assembly, please refer to [QtSpim to binary](https://github.com/Flawww/spim_to_binary)

# To do 
* More syscalls
* More instructions
* Floating point number support
* Debugging/Stepping through instructions
