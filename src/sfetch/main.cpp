#include <unistd.h>
#include <array>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <ctime>
#include "../../src/helpers/color.h"
#include "../../src/helpers/config.h"
namespace fs = std::filesystem;

std::string command(const std::string& cmd) { 
  std::array<char, 256> buffer{};
  std::string result;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return "";
  while (fgets(buffer.data(), buffer.size(), pipe)) result += buffer.data();
  pclose(pipe);
  while (!result.empty() && (result.back() == '\n' || result.back() == ' ')) result.pop_back();
  return result; 
};

std::string currentTime() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", tm);
    return buf;
};

std::string trim(std::string s) { 
  while (!s.empty() && isspace(s.front())) s.erase(s.begin()); 
  while (!s.empty() && isspace(s.back())) s.pop_back(); 
  return s;
};

// TODO  2. make color system that interact with fetch 3. make config system with toml #ALMOST 4. logos #Will make
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
};

std::string readFirstLine(const std::string& path) {
    std::ifstream file(path);
    std::string line;
    std::getline(file, line);
    return line;
};

std::string getShell() {
  const char* shell = getenv("SHELL");
  if (!shell) return "Unknown";
  std::string path(shell);
  size_t pos = path.find_last_of('/');
  if (pos != std::string::npos) return path.substr(pos + 1);
  return path; 
};

std::string getTerminal() { const char* vars[] = {
  "TERM_PROGRAM", "TERM", "COLORTERM", "Kitty", "KITTY", "Alacritty", "ALACRITTY", "Xterm", "XTERM", "St", "ST", "foot", "FOOT" };
  for (const char* var : vars) {
  const char* value = getenv(var);
  if (value && strlen(value)) return value; 
  } 
  return "Unknown";
};

std::string getResolution() {
  std::string result = command("xrandr 2>/dev/null | grep -oE '[0-9]+x[0-9]+\\+[0-9]+\\+[0-9]+' | head -n1 | cut -d '+' -f1");
  if (!result.empty()) return result;
  result = command("wayland-info 2>/dev/null | grep -m1 -oE '[0-9]+x[0-9]+'");
  if (!result.empty()) return result;
  return "Unknown";
};
    
std::string getDistro() {
    std::string content = readFile("/etc/os-release");
    size_t pos = content.find("PRETTY_NAME=");
    if (pos == std::string::npos) return "Unknown Linux";
    size_t start = pos + 13;
    size_t end = content.find('\n', start);
    std::string name = content.substr(start, end - start);
    if (!name.empty() && name.front() == '"') name.erase(0, 1);
    if (!name.empty() && name.back() == '"') name.pop_back();
    return name;
};

struct DiskInfo {
  unsigned long long total = 0;
  unsigned long long free = 0;
};
DiskInfo getDisk(const std::string& path) {
  struct statvfs stat{};
  DiskInfo result; 
  if (statvfs(path.c_str(), &stat) != 0) return result;
  result.total = static_cast<unsigned long long>(stat.f_blocks) * stat.f_frsize;
  result.free = static_cast<unsigned long long>(stat.f_bavail) * stat.f_frsize; 
  return result; }
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
};

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
};
std::string GetDiskInfo() {
  DiskInfo disk = getDisk("/");
  if (!disk.total) return "Unknown";
  unsigned long long used = disk.total - disk.free;
  int percent = static_cast<int>((used * 100.0) / disk.total);
  return humanBytes(used) + " / " + humanBytes(disk.total) + " (" + std::to_string(percent) + "%)";
}
std::string getPackages() {
  std::vector<std::pair<std::string, std::string>> managers = {
        // Arch Linux / derivatives
        {"pacman", "pacman -Qq 2>/dev/null | wc -l"},

        // Debian / Ubuntu / Mint / Pop!_OS
        {"dpkg", "dpkg-query -f '${binary:Package}\\n' -W 2>/dev/null | wc -l"},

        // Fedora / RHEL / CentOS / Rocky / Alma
        {"rpm", "rpm -qa 2>/dev/null | wc -l"},

        // openSUSE
        {"zypper", "rpm -qa 2>/dev/null | wc -l"},

        // Alpine
        {"apk", "apk info 2>/dev/null | wc -l"},

        // Gentoo
        {"equery", "equery list 2>/dev/null | wc -l"},

        // Void Linux
        {"xbps-query", "xbps-query -l 2>/dev/null | wc -l"},

        // Solus
        {"eopkg", "eopkg list-installed 2>/dev/null | wc -l"},

        // NixOS
        {"nix-env", "nix-env -q 2>/dev/null | wc -l"}
    };

    std::vector<std::string> results;

    for (const auto& [manager, cmd] : managers) {
      std::string check = command(
            "command -v " + manager + " 2>/dev/null"
        );

        if (check.empty())
            continue;

        std::string count = trim(command(cmd));

        if (count.empty() || count == "0")
            continue;

        results.push_back(count + " (" + manager + ")");
    }

    // Flatpak
    std::string flatpak = trim(command(
        "flatpak list 2>/dev/null | tail -n +1 | wc -l"
    ));

    // Snap
    std::string snap = trim(command(
        "snap list 2>/dev/null | tail -n +2 | wc -l"
    ));

    std::string result;

    if (!results.empty())
        result = results[0];

    if (!flatpak.empty() && flatpak != "0") {
        if (!result.empty())
            result += ", ";

        result += flatpak + " (flatpak)";
    }

    if (!snap.empty() && snap != "0") {
        if (!result.empty())
            result += ", ";

        result += snap + " (snap)";
    }

    return result.empty() ? "Unknown" : result;
}
std::string getDE() {
    const char* vars[] = {
        "XDG_CURRENT_DESKTOP",
        "XDG_SESSION_DESKTOP",
        "DESKTOP_SESSION"
    };

    std::string de;
    for (const char* var : vars) {
        const char* value = getenv(var);

        if (value && strlen(value)) {
            de = value;
            break;
        }
    }

    if (de.empty())
        return "Unknown";

    // KDE Plasma
    if (de.find("KDE") != std::string::npos ||
        de.find("kde") != std::string::npos ||
        de.find("PLASMA") != std::string::npos ||
        de.find("plasma") != std::string::npos)
        return "KDE Plasma";

    // GNOME
    if (de.find("GNOME") != std::string::npos ||
        de.find("gnome") != std::string::npos)
        return "GNOME";

    // XFCE
    if (de.find("XFCE") != std::string::npos ||
        de.find("xfce") != std::string::npos)
        return "Xfce";

    // Cinnamon
    if (de.find("Cinnamon") != std::string::npos ||
        de.find("cinnamon") != std::string::npos)
        return "Cinnamon";

    // MATE
    if (de.find("MATE") != std::string::npos ||
        de.find("mate") != std::string::npos)
        return "MATE";

    // LXQt
    if (de.find("LXQt") != std::string::npos ||
        de.find("lxqt") != std::string::npos)
        return "LXQt";

    // LXDE
    if (de.find("LXDE") != std::string::npos ||
        de.find("lxde") != std::string::npos)
        return "LXDE";

    // Budgie
    if (de.find("Budgie") != std::string::npos ||
        de.find("budgie") != std::string::npos)
        return "Budgie";

    // Unity
    if (de.find("Unity") != std::string::npos ||
        de.find("unity") != std::string::npos)
        return "Unity";

    // Deepin
    if (de.find("Deepin") != std::string::npos ||
        de.find("deepin") != std::string::npos)
        return "Deepin";

    // Pantheon
    if (de.find("Pantheon") != std::string::npos ||
        de.find("pantheon") != std::string::npos)
        return "Pantheon";

    // COSMIC
    if (de.find("COSMIC") != std::string::npos ||
        de.find("cosmic") != std::string::npos)
        return "COSMIC";

    return de;
}
std::string getWM() { 
    const char* wayland = getenv("WAYLAND_DISPLAY");

    if (wayland) {
        const char* desktop = getenv("XDG_CURRENT_DESKTOP");

        if (desktop) {
          std::string de = desktop;

            if (de.find("KDE") != std::string::npos)
                return "KWin";

            if (de.find("GNOME") != std::string::npos)
                return "Mutter";

            if (de.find("Hyprland") != std::string::npos)
                return "Hyprland";

            if (de.find("Sway") != std::string::npos)
                return "Sway";

            if (de.find("river") != std::string::npos)
                return "river";

            if (de.find("niri") != std::string::npos)
                return "Niri";

            if (de.find("COSMIC") != std::string::npos)
                return "COSMIC";
        }

        // Common environment variables
        if (getenv("HYPRLAND_INSTANCE_SIGNATURE"))
            return "Hyprland";

        if (getenv("SWAYSOCK"))
            return "Sway";

        if (getenv("RIVER_SOCKET"))
            return "river";

        if (getenv("NIRI_SOCKET"))
            return "Niri";
    }
    std::string result = trim(command(
        "wmctrl -m 2>/dev/null | "
        "grep '^Name:' | "
        "cut -d ':' -f2"
    ));

    if (!result.empty())
        return result;
    const char* de = getenv("XDG_CURRENT_DESKTOP");

    if (de) {
      std::string value = de;

        if (value.find("XFCE") != std::string::npos)
            return "Xfwm";

        if (value.find("KDE") != std::string::npos)
            return "KWin";

        if (value.find("GNOME") != std::string::npos)
            return "Mutter";
    }

    return "Unknown";
};

std::string getInit() {
  std::string pid1 = readFirstLine("/proc/1/comm");

    if (!pid1.empty()) {
        if (pid1 == "systemd")
            return "systemd";

        if (pid1 == "init")
            return "SysVinit";

        if (pid1 == "openrc-init")
            return "OpenRC";

        if (pid1 == "runit")
            return "runit";

        if (pid1 == "s6-svscan")
            return "s6";

        if (pid1 == "dinit")
            return "dinit";

        if (pid1 == "busybox")
            return "BusyBox init";

        return pid1;
    }
    if (fs::exists("/run/systemd/system"))
        return "systemd";

    if (fs::exists("/run/openrc"))
        return "OpenRC";

    if (fs::exists("/run/runit"))
        return "runit";

    if (fs::exists("/run/s6"))
        return "s6";

    return "Unknown";
}
std::string getArchitecture() {
    struct utsname info{};

    if (uname(&info) != 0)
        return "Unknown";

    std::string arch = info.machine;

    if (arch == "x86_64")
        return "x86_64";

    if (arch == "aarch64" ||
        arch == "arm64")
        return "ARM64";

    if (arch == "armv7l" ||
        arch == "armv7")
        return "ARMv7";

    if (arch == "armv6l")
        return "ARMv6";

    if (arch == "i386" ||
        arch == "i486" ||
        arch == "i586" ||
        arch == "i686")
        return "x86";

    if (arch == "riscv64")
        return "RISC-V 64";

    if (arch == "ppc64le")
        return "PowerPC64 LE";

    if (arch == "ppc64")
        return "PowerPC64";

    if (arch == "s390x")
        return "IBM Z";

    if (arch == "mips64")
        return "MIPS64";

    return arch;
}
void printBlocks() {
  std::cout << "  ";
  for (int i = 0; i < 8; ++i)
    std::cout << "\033[" << (40 + i) << "m" << "   ";
  std::cout << colors::RESET << "\n";
  std::cout << "  ";
  for (int i = 0; i < 8; ++i)
    std::cout << "\033[" << (100 + i) << "m" << "   ";
  std::cout << colors::RESET << "\n";
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
};

void showplan9Logo(const std::string& color = colors::GREEN) {
    std::cout << color
              << "    (\\(\\\n"
              << "   j\". ..\n"
              << "  (  . .)\n"
              << "  |   \xC2\xB0 \xC2\xA1\n"
              << "  \xC2\xBF     ;\n"
              << "  c\?\".UJ\n"
              << colors::RESET;
};
std::vector<std::string> getDifferentLogoLines(const std::string& logoName) {
    if (logoName == "kiss") {
        const char* K = "\033[40m  \033[0m";
        const char* R = "\033[41m  \033[0m";
        const char* W = "\033[47m  \033[0m";
        const char* P = "\033[45m  \033[0m";

        std::vector<std::vector<std::string>> art = {
            {K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K},
            {K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K},
            {K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K},
            {K,K,K,K,K,K,K,R,R,R,R,R,K,K,K,R,R,R,R,R,K,K,K,K,K,K},
            {K,K,K,K,K,R,R,R,R,R,R,R,K,R,R,R,R,R,R,R,R,K,K,K,K,K},
            {K,K,K,K,W,W,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,K,K,K,K},
            {K,K,K,P,W,W,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,K,K,K},
            {K,K,W,K,W,W,R,R,W,W,W,W,W,W,W,W,W,W,W,R,R,R,R,K,K,K},
            {K,K,W,K,K,R,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,R,R,K,K,K},
            {K,K,K,K,R,R,R,W,W,W,W,W,W,W,W,W,W,W,W,W,W,R,R,K,K,K},
            {K,K,K,K,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,K,K,K},
            {K,K,K,K,K,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,K,K,K,K},
            {K,K,K,K,K,K,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,K,K,K,K,K},
            {K,K,K,K,K,K,K,R,R,R,R,W,W,W,W,W,R,R,R,R,K,K,K,K,K,K},
            {K,K,K,K,K,K,K,K,R,R,R,R,R,R,R,R,R,R,R,K,K,K,K,K,K,K},
            {K,K,K,K,K,K,K,K,K,K,R,R,R,R,R,R,R,K,K,K,K,K,K,K,K,K},
            {K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K}
        };

        std::vector<std::string> lines;
        for (const auto& row : art) {
            std::string line = "  ";
            for (const auto& cell : row)
                line += cell;
            lines.push_back(line);
        }
        return lines;
    }
    return {};
}

void showLogo(const std::string& color, const std::string& logoName) {
    if (logoName == "kiss") {
        auto lines = getDifferentLogoLines(logoName);
        for (const auto& l : lines)
            std::cout << l << "\n";
    } else {
        showplan9Logo(color);
    }
}

void showInfo(const std::string& key, const std::string& value, const std::string& color = colors::BLUE) {
    std::cout << color << key << colors::RESET << "  " << value << "\n";
}

int main(int argc, char* argv[]) {
    std::string logoArg;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--logo" && i + 1 < argc)
            logoArg = argv[++i];
    }

    std::string path = std::string(getenv("HOME")) + "/.config/sfetch/config.toml";
    ensureConfig(path);
    Config cfg = loadConfig(path);

    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        std::cerr << "Error retrieving system information\n";
        return 1;
    }
    struct utsname un;
    uname(&un);
    char hostname[256];
    gethostname(hostname, sizeof hostname);

    std::string distro = getDistro();
    std::string logoName = logoArg.empty() ? "" : logoArg;
    if (logoName.empty() && distro.find("Kiss") != std::string::npos)
        logoName = "kiss";

    bool useSideBySide = cfg.logo && !logoName.empty() && logoName != "plan9";

    if (useSideBySide) {
        auto logoLines = getDifferentLogoLines(logoName);

        std::vector<std::string> infoLines;
        auto addInfo = [&](const std::string& key, const std::string& val) {
            infoLines.push_back(cfg.textcolor + key + colors::RESET + "  " + val);
        };

        infoLines.insert(infoLines.begin(), colors::BOLD + std::string(hostname) + "@" + un.sysname + colors::RESET);
        if (cfg.line)
            infoLines.insert(infoLines.begin() + 1, "=-=-=-=-=-=-=-=");
        if (cfg.os)        addInfo("os", distro + " " + getArchitecture());
        if (cfg.kernel)    addInfo("kernel", std::string(un.release));
        if (cfg.uptime)    addInfo("uptime", humanUptime(info.uptime));
        if (cfg.usedram)   addInfo("ram", humanBytes(info.totalram - info.freeram) + " / " + humanBytes(info.totalram));
        if (cfg.fullram)   addInfo("totalram", humanBytes(info.totalram));
        if (cfg.procs)     infoLines.push_back(cfg.textcolor + std::string("procs") + "  " + colors::RESET + std::to_string(info.procs));
        if (cfg.cpu)       addInfo("cpu", getCPUModel());
        if (cfg.gpu)       addInfo("gpu", getGPUModel());
        if (cfg.shell)     addInfo("shell", getShell());
        if (cfg.terminal)  addInfo("terminal", getTerminal());
        if (cfg.resolution) addInfo("resolution", getResolution());
        if (cfg.packages)  addInfo("packages", getPackages());
        if (cfg.de)        addInfo("de", getDE());
        if (cfg.wm)        addInfo("wm", getWM());
        if (cfg.init)      addInfo("init", getInit());
        if (cfg.disk)      addInfo("disk", GetDiskInfo());
        if (cfg.lastrun && !cfg.lastrunstr.empty())
            infoLines.push_back(cfg.textcolor + std::string("lastrun") + "  " + colors::RESET + cfg.lastrunstr);

        size_t maxLogo = logoLines.size();
        size_t maxInfo = infoLines.size();
        size_t rows = maxLogo > maxInfo ? maxLogo : maxInfo;

        for (size_t i = 0; i < rows; i++) {
            if (i < maxLogo)
                std::cout << logoLines[i];
            else
                std::cout << std::string(56, ' ');
            std::cout << "  ";
            if (i < maxInfo)
                std::cout << infoLines[i];
            std::cout << "\n";
        }
        if (cfg.blocks) printBlocks();
    } else {
        if (cfg.logo) showLogo(cfg.logocolor, logoName);
        std::cout << colors::BOLD << hostname << "@" << un.sysname << colors::RESET << "\n";
        if (cfg.line) std::cout << "=-=-=-=-=-=-=-=\n";
        if (cfg.os)        showInfo("os", distro + " " + getArchitecture(), cfg.textcolor);
        if (cfg.kernel)    showInfo("kernel", std::string(un.release), cfg.textcolor);
        if (cfg.uptime)    showInfo("uptime", humanUptime(info.uptime), cfg.textcolor);
        if (cfg.usedram)   showInfo("ram", humanBytes(info.totalram - info.freeram) + " / " + humanBytes(info.totalram), cfg.textcolor);
        if (cfg.fullram)   showInfo("totalram", humanBytes(info.totalram), cfg.textcolor);
        if (cfg.procs)     std::cout << cfg.textcolor << "procs" << "  " << colors::RESET << info.procs << "\n";
        if (cfg.cpu)       showInfo("cpu", getCPUModel(), cfg.textcolor);
        if (cfg.gpu)       showInfo("gpu", getGPUModel(), cfg.textcolor);
        if (cfg.shell)     showInfo("shell", getShell(), cfg.textcolor);
        if (cfg.terminal)  showInfo("terminal", getTerminal(), cfg.textcolor);
        if (cfg.resolution) showInfo("resolution", getResolution(), cfg.textcolor);
        if (cfg.packages)  showInfo("packages", getPackages(), cfg.textcolor);
        if (cfg.de)        showInfo("de", getDE(), cfg.textcolor);
        if (cfg.wm)        showInfo("wm", getWM(), cfg.textcolor);
        if (cfg.init)      showInfo("init", getInit(), cfg.textcolor);
        if (cfg.disk)      showInfo("disk", GetDiskInfo(), cfg.textcolor);
        if (cfg.lastrun && !cfg.lastrunstr.empty())
            std::cout << cfg.textcolor << "lastrun" << "  " << colors::RESET << cfg.lastrunstr << "\n";
        if (cfg.blocks) printBlocks();
    }
    try {
        toml::table t = toml::parse_file(path);             
        if (!t.contains("state"))                      
        t.insert("state", toml::table{});
        t["state"].as_table()->insert_or_assign("lastrun", currentTime());
        std::ofstream(path) << t;
     } catch (const toml::parse_error& e) { std::cerr << "Error config fail" << std::endl; 
   }  
}
