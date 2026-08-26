#include <string>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: srm <path>\n";
        return 1;
    }
    fs::path where = argv[1];
    std::error_code ec;
    fs::remove_all(where, ec);
    if (ec) {
        std::cerr << "srm: cannot remove " << where << ": " << ec.message() << "\n";
        return 1;
    }
    return 0;
}
