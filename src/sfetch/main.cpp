#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <ctime>
#include <color.h>
#include <config.h>

std::string currentTime() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", tm);
    return buf;
}

// TODO 1. Fix logo #DONE, 2. Transfer colors to color #DONE 3. make config system with toml #ALMOST 4. logos #Will make
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

std::string readFirstLine(const std::string& path) {
    std::ifstream file(path);
    std::string line;
    std::getline(file, line);
    return line;
}

std::string getDistro() {
    std::string content = readFile("/etc/os-release");
    size_t pos = content.find("NAME=");
    if (pos == std::string::npos) return "Unknown Linux";
    size_t start = pos + 5;
    size_t end = content.find('\n', start);
    std::string name = content.substr(start, end - start);
    if (!name.empty() && name.front() == '"') name.erase(0, 1);
    if (!name.empty() && name.back() == '"') name.pop_back();
    return name;
}

std::string getCPUModel() {
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("model name", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string model = line.substr(colon + 2);
                size_t freq = model.find(" @");
                if (freq != std::string::npos) model.erase(freq);
                return model;
            }
        }
    }
    return "Unknown";
}

std::string getGPUModel() {
    std::string vendor = readFirstLine("/sys/class/drm/card0/device/vendor");
    std::string device = readFirstLine("/sys/class/drm/card0/device/device");
    std::string name;
    if (vendor == "0x10de") name = "NVIDIA";
    else if (vendor == "0x1002") name = "AMD";
    else if (vendor == "0x8086") name = "Intel";
    else if (vendor == "0x15ad") name = "VMware";
    else if (vendor == "0x80ee") name = "VirtualBox";
    else name = "Unknown GPU";
    return name;
}

std::string humanBytes(unsigned long long bytes) {
    char buf[32];
    if (bytes >= 1024ULL * 1024 * 1024)
        std::snprintf(buf, sizeof buf, "%.1f GiB", bytes / 1073741824.0);
    else if (bytes >= 1024 * 1024)
        std::snprintf(buf, sizeof buf, "%.1f MiB", bytes / 1048576.0);
    else
        std::snprintf(buf, sizeof buf, "%llu B", bytes);
    return buf;
}

std::string humanUptime(long seconds) {
    long days = seconds / 86400;
    long hours = (seconds % 86400) / 3600;
    long mins = (seconds % 3600) / 60;
    char buf[32];
    if (days > 0)
        std::snprintf(buf, sizeof buf, "%ldd %ldh %ldm", days, hours, mins);
    else if (hours > 0)
        std::snprintf(buf, sizeof buf, "%ldh %ldm", hours, mins);
    else
        std::snprintf(buf, sizeof buf, "%ldm", mins);
    return buf;
}

void showplan9Logo() {
    std::cout << colors::GREEN
              << "    (\\(\\\n"
              << "   j\". ..\n"
              << "  (  . .)\n"
              << "  |   \xC2\xB0 \xC2\xA1\n"
              << "  \xC2\xBF     ;\n"
              << "  c\?\".UJ\n"
              << colors::RESET;
}

void showInfo(const std::string& key, const std::string& value) {
    std::cout << colors::BLUE << key << colors::RESET << "  " << value << "\n";
}

int main() {
    std::string path = std::string(getenv("HOME")) + "/.config/sfetch/config.toml";
    ensureConfig(path);
    Config cfg = loadConfig(path);

if (!fs::exists(path)) {
  fs::create_directories(path);
}
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        std::cerr << "Error retrieving system information\n";
        return 1;
    }
    struct utsname un;
    uname(&un);
    char hostname[256];
    gethostname(hostname, sizeof hostname);
    if (cfg.logo) {
    showplan9Logo();
    } std::cout << colors::BOLD << hostname << "@" << un.sysname << colors::RESET << "\n";
    if (cfg.line) {
    std::cout << "----------\n";
    } if (cfg.os) {
    showInfo("os", getDistro());
    } if(cfg.kernel) {
    showInfo("kernel", std::string(un.release));
    } if (cfg.uptime) {
    showInfo("uptime", humanUptime(info.uptime));
    } if (cfg.usedram) {
    showInfo("ram", humanBytes(info.totalram - info.freeram)
                + " / " + humanBytes(info.totalram));
    } if (cfg.fullram) {
    showInfo("totalram", humanBytes(info.totalram));
    } if (cfg.procs) {
    std::cout << colors::BLUE << "procs" << "  " << colors::RESET << info.procs << "\n";
    } if (cfg.cpu) {
    showInfo("cpu", getCPUModel());
    } if (cfg.gpu){
    showInfo("gpu", getGPUModel());
    }
    try {
        toml::table t = toml::parse_file(path);             
        if (!t.contains("state"))                      
        t.insert("state", toml::table{});
        t["state"].as_table()->insert_or_assign("lastrun", currentTime());
        std::ofstream(path) << t;
    } catch (const toml::parse_error& e) { std::cerr << "Error config fail" << std::endl; }
}
