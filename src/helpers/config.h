#pragma once
#include <filesystem>
#include <map>
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
std::string block_style = "square";
int block_rows = 2;
bool block_pairs = true;
std::vector<std::string> block_colors; //square, rounded
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

static const std::map<std::string, std::string> kColorNames = {
    {"black", colors::BLACK}, {"red", colors::RED}, {"green", colors::GREEN},
    {"yellow", colors::YELLOW}, {"blue", colors::BLUE}, {"magenta", colors::MAGENTA},
    {"cyan", colors::CYAN}, {"white", colors::WHITE},
    {"brightblack", colors::BRIGHT_BLACK}, {"brightred", colors::BRIGHT_RED},
    {"brightgreen", colors::BRIGHT_GREEN}, {"brightyellow", colors::BRIGHT_YELLOW},
    {"brightblue", colors::BRIGHT_BLUE}, {"brightmagenta", colors::BRIGHT_MAGENTA},
    {"brightcyan", colors::BRIGHT_CYAN}, {"brightwhite", colors::BRIGHT_WHITE},
    {"bgblack", colors::BG_BLACK}, {"bgred", colors::BG_RED},
    {"bggreen", colors::BG_GREEN}, {"bgyellow", colors::BG_YELLOW},
    {"bgblue", colors::BG_BLUE}, {"bgmagenta", colors::BG_MAGENTA},
    {"bgcyan", colors::BG_CYAN}, {"bgwhite", colors::BG_WHITE},
    {"bgbrightblack", colors::BG_BRIGHT_BLACK}, {"bgbrightred", colors::BG_BRIGHT_RED},
    {"bgbrightgreen", colors::BG_BRIGHT_GREEN}, {"bgbrightyellow", colors::BG_BRIGHT_YELLOW},
    {"bgbrightblue", colors::BG_BRIGHT_BLUE}, {"bgbrightmagenta", colors::BG_BRIGHT_MAGENTA},
    {"bgbrightcyan", colors::BG_BRIGHT_CYAN}, {"bgbrightwhite", colors::BG_BRIGHT_WHITE},
    {"gray1", colors::GRAY_1}, {"grey1", colors::GRAY_1},
    {"gray2", colors::GRAY_2}, {"grey2", colors::GRAY_2},
    {"gray3", colors::GRAY_3}, {"grey3", colors::GRAY_3},
    {"gray4", colors::GRAY_4}, {"grey4", colors::GRAY_4},
    {"gray5", colors::GRAY_5}, {"grey5", colors::GRAY_5},
    {"gray", colors::GRAY_3}, {"grey", colors::GRAY_3},
    {"orange", colors::ORANGE}, {"darkorange", colors::DARK_ORANGE},
    {"lightorange", colors::LIGHT_ORANGE},
    {"pink", colors::PINK}, {"lightpink", colors::LIGHT_PINK},
    {"hotpink", colors::HOT_PINK},
    {"purple", colors::PURPLE}, {"darkpurple", colors::DARK_PURPLE},
    {"lightpurple", colors::LIGHT_PURPLE}, {"lpurple", colors::LIGHT_PURPLE},
    {"lime", colors::LIME}, {"seagreen", colors::SEA_GREEN}, {"olive", colors::OLIVE},
    {"lightblue", colors::LIGHT_BLUE}, {"darkblue", colors::DARK_BLUE},
    {"skyblue", colors::SKY_BLUE},
    {"darkred", colors::DARK_RED}, {"crimson", colors::CRIMSON}, {"salmon", colors::SALMON},
    {"brown", colors::BROWN}, {"darkbrown", colors::DARK_BROWN},
    {"chocolate", colors::CHOCOLATE},
    {"turquoise", colors::TURQUOISE}, {"aqua", colors::AQUA},
    {"teal", colors::TEAL}, {"gold", colors::GOLD}, {"silver", colors::SILVER},
};

static std::string resolveColor(const std::string& name, const char* fallback) {
    auto it = kColorNames.find(name);
    return it != kColorNames.end() ? it->second : fallback;
}

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
        c.logocolor = resolveColor(n, colors::RESET);
        c.block_style = t["setup"]["block_style"].value_or(std::string("square"));
        c.block_rows = t["setup"]["block_rows"].value_or(c.block_rows);
        c.block_pairs = t["setup"]["block_pairs"].value_or(c.block_pairs);
        if (auto arr = t["setup"]["block_colors"].as_array())
            for (auto& e : *arr)
                if (auto s = e.value<std::string>())
                    c.block_colors.push_back(*s);
        std::string g = t["colors"]["textcolor"].value_or(std::string("blue"));
        c.textcolor = resolveColor(g, colors::RESET);
    } catch (const toml::parse_error& e) {
        std::cerr << "!!error_error_config_error!!: " << e.description() << "\n";
    }
    return c;
}

