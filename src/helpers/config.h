#pragma once
#include <filesystem>
#include <fstream>
#include <toml++/toml.hpp>
#include <color.h>
#include <iostream>

namespace fs = std::filesystem;
void ensureConfig(const std::string& path) {
    if (std::filesystem::exists(path)) return;

    fs::create_directories(fs::path(path).parent_path());
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
logo = true
line = true
lastrun = true
fullram = false
freeram = false
blocks = false
shell = true
terminal = true
resolution = true
disk = true
packages = true
de = true
wm = true
init = true

[colors]
logocolor = "white"
textcolor = "blue"
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
bool lastrun = true;
bool blocks = false;
bool shell = true;
bool terminal = true;
bool resolution = true;
bool disk = true;
bool packages = true;
bool de = true;
bool wm = true;
bool init = true;
std::string textcolor = colors::BLUE;
std::string logocolor = colors::RESET;
std::string lastrunstr;
//bool shell = true;::
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
        c.lastrun = t["setup"]["lastrun"].value_or(c.lastrun);
        c.blocks = t["setup"]["blocks"].value_or(c.blocks);
        c.shell = t["setup"]["shell"].value_or(c.shell);
        c.terminal = t["setup"]["terminal"].value_or(c.terminal);
        c.resolution = t["setup"]["resolution"].value_or(c.resolution);
        c.disk = t["setup"]["disk"].value_or(c.disk);
        c.packages = t["setup"]["packages"].value_or(c.packages);
        c.de = t["setup"]["de"].value_or(c.de);
        c.wm = t["setup"]["wm"].value_or(c.wm);
        c.init = t["setup"]["init"].value_or(c.init);
        c.lastrunstr = t["state"]["lastrun"].value_or(c.lastrunstr);

        std::string n = t["colors"]["logocolor"].value_or(std::string("white"));
        if      (n == "green") c.logocolor = colors::GREEN;
        else if (n == "blue")  c.logocolor = colors::BLUE;
        else if (n == "white") c.logocolor = colors::WHITE;
        else if (n == "pink")  c.logocolor = colors::PINK;
        else if (n == "yellow") c.logocolor = colors::YELLOW;
        else if (n == "black") c.logocolor = colors::BLACK;
        else if (n == "bgbrightyellow") c.logocolor = colors::BG_BRIGHT_YELLOW;
        else if (n == "cyan") c.logocolor = colors::CYAN;
        else if (n == "brown" || n == "chocolate") c.logocolor = colors::CHOCOLATE;
        else if (n == "red") c.logocolor = colors::RED;
        else if (n == "lpurple") c.logocolor = colors::LIGHT_PURPLE;
        else if (n == "purple") c.logocolor = colors::PURPLE;
        else if (n == "aqua") c.logocolor = colors::AQUA;
        else if (n == "magenta") c.logocolor = colors::MAGENTA;
        else                   c.logocolor = colors::RESET;
        std::string g = t["colors"]["textcolor"].value_or(std::string("blue"));
        if      (g == "green") c.textcolor = colors::GREEN;
        else if (g == "blue")  c.textcolor = colors::BLUE;
        else if (g == "white") c.textcolor = colors::WHITE;
        else if (g == "pink")  c.textcolor = colors::PINK;
        else if (g == "yellow") c.textcolor = colors::YELLOW;
        else if (g == "black") c.textcolor = colors::BLACK;
        else if (g == "bgbrightyellow") c.textcolor = colors::BG_BRIGHT_YELLOW;
        else if (g == "cyan") c.textcolor = colors::CYAN;
        else if (g == "brown" || g == "chocolate") c.textcolor = colors::CHOCOLATE;
        else if (g == "red") c.textcolor = colors::RED;
        else if (g == "lpurple") c.textcolor = colors::LIGHT_PURPLE;
        else if (g == "purple") c.textcolor = colors::PURPLE;
        else if (g == "aqua") c.textcolor = colors::AQUA;
        else if (g == "magenta") c.textcolor = colors::MAGENTA;
        else                   c.textcolor = colors::RESET;
    } catch (const toml::parse_error& e) {
        std::cerr << "!!error_error_config_error!!: " << e.description() << "\n";
    }
    return c;
}

