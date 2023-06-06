# PIE Checker

A program to check if a given file is an executable file and has the Position Independent Executable flag turned on.

## Installation

    module add cmake/latest gcc/10.3.0
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
    make

## Usage

The program accepts files on the standard input line by line until it receives the End Of File character. Files not necessarily
will be processed in the order they are received from the standard input.

### Use example

    find * -type f | ./pie_checker

### Example output

    file;type;arch;endianness;is_pie
    CMakeFiles/pie_checker.dir/main.cpp.o;elf_relocatable_file;64_bit;lsb;not_pie
    CMakeFiles/pie_checker.dir/Worker.cpp.o;elf_relocatable_file;64_bit;lsb;not_pie
    CMakeFiles/pie_checker.dir/WorkerManager.cpp.o;elf_relocatable_file;64_bit;lsb;not_pie
    CMakeFiles/pie_checker.dir/Job.cpp.o;elf_relocatable_file;64_bit;lsb;not_pie
    CMakeFiles/3.22.1/CMakeDetermineCompilerABI_C.bin;elf_shared_object;64_bit;lsb;pie
    CMakeFiles/3.22.1/CompilerIdC/a.out;elf_shared_object;64_bit;lsb;pie
    CMakeFiles/3.22.1/CompilerIdCXX/a.out;elf_shared_object;64_bit;lsb;pie
    CMakeFiles/3.22.1/CMakeDetermineCompilerABI_CXX.bin;elf_shared_object;64_bit;lsb;pie
    pie_checker;elf_shared_object;64_bit;lsb;pie
