#include "WorkerManager.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <unistd.h>

int main(int argc, const char *argv[]) {
    const size_t nproc = sysconf(_SC_NPROCESSORS_ONLN);
    WorkerManager manager(nproc);

    std::cout << "file;type;arch;endianness;is_pie\n";

    std::string line;
    while (std::getline(std::cin, line)) {
        manager.PushFile(line);
    }
    return EXIT_SUCCESS;
}
