#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <unistd.h>
#include <string>

int main(int argc, char **argv) {

    if (argc != 2) { 
        std::cerr << "Usage: kill <process ID\n";
        return 0;
    }

    pid_t process = std::stoi(argv[1]);
    if (kill(process, 9)) {
        std::cerr << strerror(errno) << '\n';
        return 0;
    }
}

