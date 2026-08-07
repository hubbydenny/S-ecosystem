#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

// TODO 1. Fix logo, 2. Transfer colors to color 3. make config system with toml 4. logos
const char* RESET = "\033[0m";
const char* GREEN = "\033[1;32m";
const char* BLUE  = "\033[1;34m";
const char* BOLD  = "\033[1m";

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
    if (name != "Unknown GPU") {
    return name;
    }
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

void showLogo() {
    std::cout << GREEN
              << "    (\\(\\\n"
              << "   j\". ..\n"
              << "  (  . .)\n"
              << "  |   \xC2\xB0 \xC2\xA1\n"
              << "  \xC2\xBF     ;\n"
              << "  c\".?UJ\n"
              << RESET;
}

void showInfo(const std::string& key, const std::string& value) {
    std::cout << BLUE << key << RESET << "  " << value << "\n";
}

int main(void) {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        std::cerr << "Error retrieving system information\n";
        return 1;
    }

    struct utsname un;
    uname(&un);

    char hostname[256];
    gethostname(hostname, sizeof hostname);

    showLogo();
    std::cout << BOLD << hostname << "@" << un.sysname << RESET << "\n";
    std::cout << "----------\n";
    showInfo("os", getDistro());
    showInfo("kernel", std::string(un.release));
    showInfo("uptime", humanUptime(info.uptime));
    showInfo("ram", humanBytes(info.totalram - info.freeram)
                + " / " + humanBytes(info.totalram));
    showInfo("cpu", getCPUModel());
    showInfo("gpu", getGPUModel()); 

    return 0;
}
