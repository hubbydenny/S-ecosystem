#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: smv <from> <to>\n";
        return 1;
    }
    fs::path from = argv[1];
    fs::path to = argv[2];
    std::error_code ec;
    fs::rename(from, to, ec);
    if (ec) {
        std::cerr << "smv: cannot rename " << from << ": " << ec.message() << "\n";
        return 1;
    }
    return 0;
}
