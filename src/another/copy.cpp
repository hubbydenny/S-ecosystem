#include <string>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: scopy <from> <to>\n";
        return 1;
    }
    fs::path from = argv[1];
    fs::path to = argv[2];
    std::error_code ec;
    fs::copy(from, to, fs::copy_options::recursive, ec);
    if (ec) {
        std::cerr << "copy failed: " << ec.message() << "\n";
        return 1;
    }
    return 0;
}
