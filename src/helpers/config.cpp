#include <filesystem>
#include <fstream>
#include <toml/toml.hpp>
#include <color.h>
void ensureConfig(const std::string& path) {
    if (std::filesystem::exists(path)) return;

    std::ofstream f(path);
    f << R"(# sfetch config is generated, edit it and type sfetch in shell

[colors]
logo = "plan9"
key = "blue"

[setup]
show_cpu = true
show_gpu = true
logo = true
)";
    std::cout << "~ created default config: " << path << "\n";
}
struct config { 
bool os = true;
bool kerner = true;
bool uptime = true;
bool usedram = true;
bool procs = true;
bool cpu = true;
bool gpu = true;
bool fullram = false;
bool freeram = false;
//bool shell = true;
std::string colorlogo = ;
}

