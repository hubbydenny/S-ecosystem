#pragma once
#include <filesystem>
#include <fstream>
#include <toml++/toml.hpp>
#include <color.h>
#include <iostream>

namespace fs = std::filesystem;
void ensureConfig(const std::string& path) {
    if (std::filesystem::exists(path)) return;

    std::ofstream f(path);
    f << R"(# sfetch config is generated, edit it and type sfetch in shell

[setup]
os = true
kernel = true
uptime = true
usedram = true
procs = true
cpu = true
gpu = true
fullram = false
freeram = false
logo = true
line = true

[colors]
logocolor = "green"
)";
    std::cout << "~ created default config: " << path << "\n";
};
struct Config { 
bool os = true;
bool kernel = true;
bool uptime = true;
bool usedram = true;
bool procs = true;
bool cpu = true;
bool gpu = true;
bool logo = true;
bool line = true;
bool fullram = false;
bool freeram = false;
std::string logocolor = colors::RESET;
std::string lastrun;
//bool shell = true;
};
Config loadConfig(const std::string& path) {
    Config c;
    std::ifstream f(path);
    if (!f) return c;   
    try {
        toml::table t = toml::parse(f, path);
        c.os      = t["setup"]["os"].value_or(c.os);
        c.kernel  = t["setup"]["kernel"].value_or(c.kernel);
        c.uptime  = t["setup"]["uptime"].value_or(c.uptime);
        c.usedram = t["setup"]["usedram"].value_or(c.usedram);
        c.procs   = t["setup"]["procs"].value_or(c.procs);
        c.cpu     = t["setup"]["cpu"].value_or(c.cpu);
        c.gpu     = t["setup"]["gpu"].value_or(c.gpu);
        c.fullram = t["setup"]["fullram"].value_or(c.fullram);
        c.freeram = t["setup"]["freeram"].value_or(c.freeram);
        c.logo    = t["setup"]["logo"].value_or(c.logo);
        c.line    = t["setup"]["line"].value_or(c.line);

        std::string n = t["colors"]["logocolor"].value_or(std::string("green"));
        if      (n == "green") c.logocolor = colors::GREEN;
        else if (n == "blue")  c.logocolor = colors::BLUE;
        else                   c.logocolor = colors::RESET;
        
    } catch (const toml::parse_error& e) {
        std::cerr << "!!error_error_config_error!!: " << e.description() << "\n";
    }
    return c;
}

