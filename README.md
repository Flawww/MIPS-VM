# MIPS Virtual Machine
Cross-platform virtual machine for MIPS, written in C++.

# Usage
After starting the VM, enter the name of the MIPS program to run. The MIPS program at very least needs to have have a `.text` section. The files should be the (compiled) binary instructions for the program. Each section has a separate file, with different suffixes (`.text`,  `.data`,  `.ktext`,  `.kdata`).

To easily generate these binary files from MIPS assembly, please refer to [QtSpim to binary](https://github.com/Flawww/spim_to_binary)

# Compilation
Requires a compiler that supports C++17 or newer.
## Windows
Either:
* Open the project solution in Visual Studio and build normally through Visual Studio with MSVC.

or

* Run `build.bat` to build the project with GCC

## Linux
GCC needs to be installed. 
* Run `build.sh` to build the project. 

# To do 
* Rest of the instructions not yet supported (mostly float/double related)
* Floating point number support
* Debugging/Stepping through instructions
