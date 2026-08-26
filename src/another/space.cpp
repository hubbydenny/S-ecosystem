#include <string>
#include <filesystem>
#include <iostream>
#include <cstdio>
#include <cstdint>

namespace fs = std::filesystem;

static std::string human(uintmax_t bytes) {
    const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double v = bytes;
    int i = 0;
    while (v >= 1024.0 && i < 5) { v /= 1024.0; i++; }
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.1f %s", v, units[i]);
    return buf;
}

int main(int argc, char* argv[]) {
    fs::path target;
    if (argc >= 2) target = argv[1];
    else target = fs::current_path();

    std::error_code ec;
    fs::space_info s = fs::space(target, ec);
    if (ec) {
        std::cerr << "space: cannot stat " << target << ": " << ec.message() << "\n";
        return 1;
    }
    std::cout << "path:      " << target << "\n";
    std::cout << "capacity:  " << human(s.capacity) << "\n";
    std::cout << "free:      " << human(s.free) << "\n";
    std::cout << "available: " << human(s.available) << "\n";
    return 0;
}
