#include <unistd.h>
#include <array>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <sys/utsname.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/sysctl.h>
#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/user.h>
#endif
#endif
#include <sys/statvfs.h>
#include <sys/time.h>

struct SysStats {
    long uptime = 0;
    unsigned long long totalram = 0;
    unsigned long long freeram = 0;
    unsigned int procs = 0;
};

static bool getSysStats(SysStats& s) {
#ifdef __linux__
    struct sysinfo i;
    if (sysinfo(&i) != 0) return false;
    s.uptime = i.uptime;
    s.totalram = i.totalram;
    s.freeram = i.freeram;
    s.procs = i.procs;
    return true;
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
    struct timespec ts;
    if (clock_gettime(CLOCK_UPTIME, &ts) == 0) {
        s.uptime = ts.tv_sec;
    } else {
        struct timeval bt {};
        size_t blen = sizeof(bt);
        if (sysctlbyname("kern.boottime", &bt, &blen, nullptr, 0) == 0) {
            struct timeval now {};
            gettimeofday(&now, nullptr);
            s.uptime = (long)(now.tv_sec - bt.tv_sec);
        }
    }
    size_t len = sizeof(s.totalram);
    sysctlbyname("hw.physmem", &s.totalram, &len, nullptr, 0);
    len = sizeof(s.freeram);
    sysctlbyname("hw.usermem", &s.freeram, &len, nullptr, 0);
#if defined(__FreeBSD__) || defined(__DragonFly__)
    len = 0;
    if (sysctlbyname("kern.proc.all", nullptr, &len, nullptr, 0) == 0)
        s.procs = (unsigned)(len / sizeof(struct kinfo_proc));
#endif
    return true;
#else
    return false;
#endif
}
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
  return result; 
}

std::string getCPUModel() {
#ifndef __linux__
    {
        FILE* p = popen("sysctl -n hw.model 2>/dev/null", "r");
        if (p) {
            char buf[256] = {0};
            if (fgets(buf, sizeof buf, p)) {
                pclose(p);
                std::string m(buf);
                while (!m.empty() && (m.back() == '\n' || m.back() == '\r' || m.back() == ' ')) m.pop_back();
                if (!m.empty()) return m;
            } else pclose(p);
        }
    }
#endif
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
#ifndef __linux__
    {
        FILE* p = popen("pciconf -lv 2>/dev/null", "r");
        if (p) {
            char buf[1024];
            while (fgets(buf, sizeof buf, p)) {
                std::string line(buf);
                if (line.find("class=0x03") == std::string::npos) continue;
                size_t v = line.find("vendor=0x");
                if (v == std::string::npos) continue;
                std::string id = line.substr(v + 9, 4);
                pclose(p);
                if (id == "10de") return "NVIDIA";
                if (id == "1002") return "AMD";
                if (id == "8086") return "Intel";
                if (id == "15ad") return "VMware";
                if (id == "80ee") return "VirtualBox";
                return "Unknown GPU";
            }
            pclose(p);
        }
    }
#endif
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
        {"pacman", "pacman -Qq 2>/dev/null | wc -l"},
        {"dpkg", "dpkg-query -f '${binary:Package}\\n' -W 2>/dev/null | wc -l"},
        {"rpm", "rpm -qa 2>/dev/null | wc -l"},
        {"zypper", "rpm -qa 2>/dev/null | wc -l"},
        {"apk", "apk info 2>/dev/null | wc -l"},
        {"equery", "equery list 2>/dev/null | wc -l"},
        {"xbps-query", "xbps-query -l 2>/dev/null | wc -l"},
        {"eopkg", "eopkg list-installed 2>/dev/null | wc -l"},
        {"nix-env", "nix-env -q 2>/dev/null | wc -l"},
        {"pkg", "pkg info 2>/dev/null | wc -l"},
        {"pkgin", "pkgin list 2>/dev/null | tail -n +2 | wc -l"},
        {"pkg_info", "pkg_info 2>/dev/null | wc -l"}
    };

    std::vector<std::string> results;

    for (const auto& [manager, cmd] : managers) {
      std::string check = command("command -v " + manager + " 2>/dev/null");
      if (check.empty()) continue;

      std::string count = trim(command(cmd));
      if (count.empty() || count == "0") continue;

      results.push_back(count + " (" + manager + ")");
    }

    std::string flatpak = trim(command("flatpak list 2>/dev/null | tail -n +1 | wc -l"));
    std::string snap = trim(command("snap list 2>/dev/null | tail -n +2 | wc -l"));

    std::string result;
    if (!results.empty()) result = results[0];

    if (!flatpak.empty() && flatpak != "0") {
        if (!result.empty()) result += ", ";
        result += flatpak + " (flatpak)";
    }

    if (!snap.empty() && snap != "0") {
        if (!result.empty()) result += ", ";
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

    if (de.empty()) return "Unknown";

    if (de.find("KDE") != std::string::npos || de.find("kde") != std::string::npos ||
        de.find("PLASMA") != std::string::npos || de.find("plasma") != std::string::npos)
        return "KDE Plasma";
    if (de.find("GNOME") != std::string::npos || de.find("gnome") != std::string::npos) return "GNOME";
    if (de.find("XFCE") != std::string::npos || de.find("xfce") != std::string::npos) return "Xfce";
    if (de.find("Cinnamon") != std::string::npos || de.find("cinnamon") != std::string::npos) return "Cinnamon";
    if (de.find("MATE") != std::string::npos || de.find("mate") != std::string::npos) return "MATE";
    if (de.find("LXQt") != std::string::npos || de.find("lxqt") != std::string::npos) return "LXQt";
    if (de.find("LXDE") != std::string::npos || de.find("lxde") != std::string::npos) return "LXDE";
    if (de.find("Budgie") != std::string::npos || de.find("budgie") != std::string::npos) return "Budgie";
    if (de.find("Unity") != std::string::npos || de.find("unity") != std::string::npos) return "Unity";
    if (de.find("Deepin") != std::string::npos || de.find("deepin") != std::string::npos) return "Deepin";
    if (de.find("Pantheon") != std::string::npos || de.find("pantheon") != std::string::npos) return "Pantheon";
    if (de.find("COSMIC") != std::string::npos || de.find("cosmic") != std::string::npos) return "COSMIC";

    return de;
}

std::string getWM() { 
    const char* wayland = getenv("WAYLAND_DISPLAY");

    if (wayland) {
        const char* desktop = getenv("XDG_CURRENT_DESKTOP");
        if (desktop) {
            std::string de = desktop;
            if (de.find("KDE") != std::string::npos) return "KWin";
            if (de.find("GNOME") != std::string::npos) return "Mutter";
            if (de.find("Hyprland") != std::string::npos) return "Hyprland";
            if (de.find("Sway") != std::string::npos) return "Sway";
            if (de.find("river") != std::string::npos) return "river";
            if (de.find("niri") != std::string::npos) return "Niri";
            if (de.find("COSMIC") != std::string::npos) return "COSMIC";
        }
        if (getenv("HYPRLAND_INSTANCE_SIGNATURE")) return "Hyprland";
        if (getenv("SWAYSOCK")) return "Sway";
        if (getenv("RIVER_SOCKET")) return "river";
        if (getenv("NIRI_SOCKET")) return "Niri";
    }

    std::string result = trim(command("wmctrl -m 2>/dev/null | grep '^Name:' | cut -d ':' -f2"));
    if (!result.empty()) return result;

    const char* de = getenv("XDG_CURRENT_DESKTOP");
    if (de) {
        std::string value = de;
        if (value.find("XFCE") != std::string::npos) return "Xfwm";
        if (value.find("KDE") != std::string::npos) return "KWin";
        if (value.find("GNOME") != std::string::npos) return "Mutter";
    }

    return "Unknown";
};

std::string getInit() {
    std::string pid1 = readFirstLine("/proc/1/comm");
#ifndef __linux__
    if (pid1.empty()) {
        pid1 = trim(command("ps -o comm= -p 1 2>/dev/null"));
        if (!pid1.empty()) return pid1;
    }
#endif
    if (!pid1.empty()) {
        if (pid1 == "systemd") return "systemd";
        if (pid1 == "init") return "SysVinit";
        if (pid1 == "openrc-init") return "OpenRC";
        if (pid1 == "runit") return "runit";
        if (pid1 == "s6-svscan") return "s6";
        if (pid1 == "dinit") return "dinit";
        if (pid1 == "busybox") return "BusyBox init";
        return pid1;
    }
    if (fs::exists("/run/systemd/system")) return "systemd";
    if (fs::exists("/run/openrc")) return "OpenRC";
    if (fs::exists("/run/runit")) return "runit";
    if (fs::exists("/run/s6")) return "s6";

    return "Unknown";
}

std::string getBattery() {
#ifdef __linux__
    fs::path ps("/sys/class/power_supply");
    if (!fs::is_directory(ps)) return "";
    for (const auto& entry : fs::directory_iterator(ps)) {
        std::string name = entry.path().filename().string();
        if (name.find("BAT") != std::string::npos) {
            std::string cap = trim(readFile(entry.path() / "capacity"));
            std::string status = trim(readFile(entry.path() / "status"));
            if (cap.empty()) return "";
            return cap + "%" + (status.empty() ? "" : " [" + status + "]");
        }
    }
    return "";
#elif defined(__FreeBSD__) || defined(__DragonFly__)
    std::string life = trim(command("sysctl -n hw.acpi.battery.life 2>/dev/null"));
    if (life.empty()) return "";
    std::string status = trim(command("sysctl -n hw.acpi.battery.state 2>/dev/null"));
    std::string st;
    if (status == "1") st = " [charging]";
    else if (status == "2") st = "";
    return life + "%" + st;
#elif defined(__OpenBSD__) || defined(__NetBSD__)
    std::string life = trim(command("sysctl -n hw.acpi.battery.life 2>/dev/null"));
    if (life.empty()) return "";
    return life + "%";
#else
    return "";
#endif
}



std::string getArchitecture() {
    struct utsname info{};
    if (uname(&info) != 0) return "Unknown";

    std::string arch = info.machine;
    if (arch == "x86_64") return "x86_64";
    if (arch == "aarch64" || arch == "arm64") return "ARM64";
    if (arch == "armv7l" || arch == "armv7") return "ARMv7";
    if (arch == "armv6l") return "ARMv6";
    if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686") return "x86";
    if (arch == "riscv64") return "RISC-V 64";
    if (arch == "ppc64le") return "PowerPC64 LE";
    if (arch == "ppc64") return "PowerPC64";
    if (arch == "s390x") return "IBM Z";
    if (arch == "mips64") return "MIPS64";

    return arch;
}

void printBlocks(const std::string& indent = "  ", const Config& cfg = Config{}) {
  bool rounded = (cfg.block_style == "rounded" || cfg.block_style == "circle");
  static const char* DOT = "●●";
  static const char* BLOCK = "███";
  std::string dotStr = cfg.block_pairs ? DOT : std::string("●");
  auto cell = [&](const std::string& col) {
    return col + (rounded ? dotStr + " " : BLOCK) + colors::RESET;
  };
  std::vector<std::string> cells;
  if (cfg.block_colors.empty()) {
    static const char* def_rounded[16] = {
        "red", "orange", "yellow", "lime", "aqua", "skyblue", "purple", "hotpink",
        "lightpink", "lightorange", "gold", "olive", "turquoise", "lightblue", "lpurple", "silver"
    };
    if (rounded) {
      for (int i = 0; i < 16; ++i)
        cells.push_back(cell(resolveColor(def_rounded[i], colors::RESET)));
    } else {
      for (int i = 0; i < 8; ++i)
        cells.push_back(std::string("\033[") + std::to_string(40 + i) + "m" + "   " + colors::RESET);
      for (int i = 0; i < 8; ++i)
        cells.push_back(std::string("\033[") + std::to_string(100 + i) + "m" + "   " + colors::RESET);
    }
  } else {
    for (const auto& n : cfg.block_colors)
      cells.push_back(cell(resolveColor(n, colors::RESET)));
  }
  int rows = cfg.block_rows > 0 ? cfg.block_rows : 2;
  size_t per_row = (cells.size() + rows - 1) / rows;
  for (size_t r = 0; r < cells.size(); r += per_row) {
    std::cout << indent;
    for (size_t i = r; i < std::min(r + per_row, cells.size()); ++i)
      std::cout << cells[i];
    std::cout << "\n";
  }
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

static size_t visWidth(const std::string& s) {
    size_t w = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\033') { while (i < s.size() && s[i] != 'm') i++; continue; }
        if ((unsigned char)s[i] == 0xE2 && i + 2 < s.size() &&
            (unsigned char)s[i+1] == 0x96 && (unsigned char)s[i+2] == 0x88) {
            w += 2;
            i += 2;
            continue;
        }
        if ((unsigned char)s[i] >= 0xC0) continue;
        w++;
    }
    return w;
}

static std::vector<std::string> parseLogoArt(const std::string& art) {
    std::vector<std::string> lines;
    std::istringstream ss(art);
    std::string raw;
    while (std::getline(ss, raw)) {
        if (raw.empty()) continue;
        std::string line;
        for (size_t i = 0; i < raw.size(); i++) {
            if (raw[i] == '$' && i + 1 < raw.size() && raw[i + 1] >= '1' && raw[i + 1] <= '5') {
                i++;
            } else {
                line += raw[i];
            }
        }
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> getDifferentLogoLines(const std::string& logoName) {
   if (logoName == "kiss" || logoName == "Kiss") {
        const std::string R  = "\033[38;2;255;0;0m██";
        const std::string W  = "\033[38;2;255;255;255m██";
        const std::string P  = "\033[38;2;210;0;70m██";
        const std::string S  = "  ";
        const std::string RS = "\033[0m";

        const std::vector<std::string> logo = {
            S+S+S+S+S + R+R+R+R + S+S+S + R+R+R+R + S+S+S+S+S,
            S+S+S + W+W + R+R+R+R+R+R+R+R+R+R+R+R + S+S+S+S,
            S+S + P+W+W + R+R+R+R+R+R+R+R+R+R+R+R + S+S+S+S,
            S + R+W+R+R + W+W+W+W+W+W+W+W+W+W + R+R+R+R + S+S,
            R+R+R+R + W+W+W+W+W+W+W+W+W+W+W+W + R+R+R+R + S,
            R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R + S,
            S + R+R+R+R+R+R + W+W+W+W+W + R+R+R+R+R+R + S+S,
            S+S + R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R + S+S+S,
            S+S+S+S+S + R+R+R+R+R+R+R+R+R+R + S+S+S+S+S+S
        };

        std::vector<std::string> lines;
        lines.reserve(logo.size());
        for (const auto& row : logo) {
            lines.push_back("  " + row + RS);
        }
        return lines;
    }    if (logoName == "alpine") {
        std::string art =
R"(
    /\ /\
   / / \ \
  / /   \ \
 / /     \ \
         \
)";
        return parseLogoArt(art);
    }
    if (logoName == "alpine2") { //if u want this custom logo for alpine
        std::string art =
R"(
$1   /\ /\
  /$2/ $1\  \
 /$2//  $1\  \
/$2//    $1\  \
$2//      $1\  \
         \
)";
        return parseLogoArt(art);
    }
    if (logoName == "arch") {
        std::string art =
R"(
      /\
     /  \
    /    \
   /      \
$2  /   ,,   \
 /   |  |   \
/_-''    ''-_\
)";
        return parseLogoArt(art);
    }
    if (logoName == "artix") {
        std::string art =
R"(
            '
           'A'
          'ooo'
         'ookxo'
         `ookxxo'
       '.   `ooko'
      'ooo`.   `oo'
     'ooxxxoo`.   `'
    'ookxxxkooo.`   .
   'ookxxkoo'`   .'oo'
  'ooxoo'`     .:ooxxo'
 'io'`             `'oo'
'`                     `'
)";
        return parseLogoArt(art);
    }
    if (logoName == "artix2") {
        std::string art =
R"(
      /\
     /  \
    /`'.,\
   /     ',
  /      ,`\
 /   ,.'`.  \
/.,'`     `'.\
)";
        return parseLogoArt(art);
    }
    if (logoName == "bedrock") {
        std::string art =
R"(
_________
| $2__     $1 |
| $2\ \___ $1 |
| $2 \  _ \$1 |
| $2  \___/$1 |
|_________|
)";
        return parseLogoArt(art);
    }
    if (logoName == "chimera") {
        std::string art =
R"(
$3XXXXX $1I:
$3XXX' $1,I;
$3XX $1,f""'.,,,,
$2,, $1I:   ;P"""
$2XX $1`t...f' $4jj
$2XXX. $1`"' $4.XXX
$2OOOOOC $4lXXXXX
)";
        return parseLogoArt(art);
    }
    if (logoName == "chimera2") {
        std::string art =
R"(
888888888888  $2888
$1888888888888  $2888
$1888888888888  $2888
$188888888P"' $2_,888
$1888888P' $2,jd88888
$188888P  $2d88P'
$18888b  $2j88'         xxxxxxxxxx
$3_____  $218{          8888888888
$38888b. $2l88,        ,88" $3______
$3888888  $218b,_    ,d88P  $3888888
$3888888b. $2`188bwwd88P' $3,d888888
$388888888b._ $2`"^^"'`$3.,d88888888
$3888888888888bo  od888888888888
$388888888888888  88888888888888
$388888888888888  88888888888888
)";
        return parseLogoArt(art);
    }
    if (logoName == "debian") {
        std::string art =
R"(
  _____
 /  __ \
|  /    |
|  \___-
-_
  --_
)";
        return parseLogoArt(art);
    }
    if (logoName == "exherbo") {
        std::string art =
R"(
$2 ,
OXo.
NXdX0:    .cok0KXNNXXK0ko:.
KX  '0XdKMMK;.xMMMk, .0MMMMMXx;  ...
'NO..xWkMMx   kMMM    cMMMMMX,NMWOxOXd.
  cNMk  NK    .oXM.   OMMMMO. 0MMNo  kW.
  lMc   o:       .,   .oKNk;   ;NMMWlxW'
 ;Mc    ..   .,,'    .0M$1g;$2WMN'dWMMMMMMO
 XX        ,WMMMMW.  cM$1cfli$2WMKlo.   .kMk
.Mo        .WM$1GD$2MW.   XM$1WO0$2MMk        oMl
,M:         ,XMMWx::,''oOK0x;          NM.
'Ml      ,kNKOxxxxxkkO0XXKOd:.         oMk
 NK    .0Nxc$3:::::::::::::::$2fkKNk,      .MW
 ,Mo  .NXc$3::$2qXWXb$3::::::::::$2oo$3::$2lNK.    .MW
  ;Wo oMd$3:::$2oNMNP$3::::::::$2oWMMMx$3:$2c0M;   lMO
   'NO;W0c$3:::::::::::::::$2dMMMMO$3::$2lMk  .WM'
     xWONXdc$3::::::::::::::$2oOOo$3::$2lXN. ,WMd
      'KWWNXXK0Okxxo,$3:::::::$2,lkKNo  xMMO
        :XMNxl,';:lodxkOO000Oxc. .oWMMo
          'dXMMXkl;,.        .,o0MMNo'
             ':d0XWMMMMWNNNNMMMNOl'
                   ':okKXWNKkl'
)";
        return parseLogoArt(art);
    }
    if (logoName == "gentoo") {
        std::string art =
R"(
 _-----_
(       \
\    0   \
 $2\        )
 /      _/
(     _-
\____-
)";
        return parseLogoArt(art);
    }
    if (logoName == "gnu") {
        std::string art =
R"(
    _-`````-,           ,- '- .
  .'   .- - |          | - -.  `.
 /.'  /                     `.   \
:/   :      _...   ..._      ``   :
::   :     /._ .`:'_.._\.    ||   :
::    `._ ./  ,`  :    \ . _.''   .
`:.      /   |  -.  \-. \\_      /
  \:._ _/  .'   .@)  \@) ` `\ ,.'
     _/,--'       .- .\,-.`--`.
       ,'/''     (( \ `  )
        /'/'  \    `-'  (
         '/''  `._,-----'
          ''/'    .,---'
           ''/'      ;:
             ''/''  ''/
               ''/''/''
                 '/'/'
                  `;
)";
        return parseLogoArt(art);
    }
    if (logoName == "guix") {
        std::string art =
R"(
 ..                             `.
 `--..```..`           `..```..--`
   .-:///-:::.       `-:::///:-.
      ````.:::`     `:::.````
           -//:`    -::-
            ://:   -::-
            `///- .:::`
             -+++-:::.
              :+/:::-
              `-....`
)";
        return parseLogoArt(art);
    }
    if (logoName == "haiku") {
        std::string art =
R"(
       ,^,
      /   \
*--_ ;     ; _--*
\   '"     "'   /
 '.           .'
.-'"         "'-.
 '-.__.   .__.-'
       |_|
)";
        return parseLogoArt(art);
    }
    if (logoName == "parabola") {
        std::string art =
R"(
                          `.-.    `.
                   `.`  `:++.   `-+o+.
             `` `:+/. `:+/.   `-+oooo+
        ``-::-.:+/. `:+/.   `-+oooooo+
    `.-:///-  ..`   .-.   `-+oooooooo-
 `..-..`                 `+ooooooooo:
``                        :oooooooo/
                          `ooooooo:
                          `oooooo:
                          -oooo+.
                          +ooo/`
                         -ooo-
                        `+o/.
                        /+-
                       //`
                      -.
)";
        return parseLogoArt(art);
    }
    if (logoName == "devuan" || logoName == "Devuan") {
        std::string art = 
R"(
 ..:::.
     ..-==-
        .+#:    
         =@@    
      :+%@#:    
.:=+#@@%*:      
#@@@#=:
)";
       return parseLogoArt(art);
}
    if (logoName == "fedora" || logoName == "fedora") {
       std::string art =
R"(
        ,'''''.  
       |   ,.  | 
       |  |  |.| 
  ,....|  |..   
.'  ,_;|   ..'   
|  |   |  |      
|  ',_,'  |      
 '.     ,'       
   '''''         
)"; return parseLogoArt(art);
  }
if (logoName == "ubuntu" || logoName == "Ubuntu") {
      std::string art =
R"(
  
         _    
     ---(_)   
 _/  ---  \\   
(_) |   |  |  
  \\  --- _/   
    ---(_)   
)";
  return parseLogoArt(art);
  }
if (logoName == "Derive" || logoName == "Derive") {
      std::string art =
R"(
                               ===o######o==o
                            oo##-#############=
                          -oo=#.#   |  /   #.o--###-
                         #o-..=  -- | ---   .-==#=o
                        o##=#-./   ||___/  "##=.#==##=
                        .##=#.-\\___|\\__    o=-oo=##=
                         #=#.=##-o-== ==#=o#.#o#o=
                          -=  -=#-o===o.##--o#-#.
                          -o#oo--=o=o=#-.o###==
                         -==##o=--==o==--=o
                        o.
             =o###o###==
          o-o= #=##-######-
          =o.=-.o####- -###-
        =.=# #-####.    ###-
       =##o ##=###-     .##--
        =..##o###.      =##-
      -=# =-.###        ###=
        -=-=### =#=o#= o##.
         =####=####o#=o##o
          -o##=##o-#####=o
        -=o#o-o#######o-
       o=o--ooooooo-
        ===o-=o===o
      -=.oo-ooo=o=
    ===.---ooo==#o
   o--#--.-==oo.#-#.
  oo-=#=#o==###oo==
  =#oooo#o===-
  -o#oo#### o=
  =#o=oo##=#o-
  oo###o=oo.
 .###o##-o#=o
 =ooo=-ooooooo=o
 o. ..-=-.-
)";
  return parseLogoArt(art);
  }
  if (logoName == "raspbian" || logoName == "Raspbian" || logoName == "Raspberry" || logoName == "raspberry" || logoName == "Raspberry Pi OS") {
        std::string art =
R"(
   $2`.::///+:/-.        --///+//-:`
 `+oooooooooooo:   `+oooooooooooo:
  /oooo++//ooooo:  ooooo+//+ooooo.
  `+ooooooo:-:oo-  +o+::/ooooooo:
   `:oooooooo+``    `.oooooooo+-
     `:++ooo/.        :+ooo+/.`$1
        ...`  `.----.` ``..
     .::::-``:::::::::.`-:::-`
    -:::-`   .:::::::-`  `-:::-
   `::.  `.--.`  `` `.---.``.::`
       .::::::::`  -::::::::` `
 .::` .:::::::::- `::::::::::``::.
-:::` ::::::::::.  ::::::::::.`:::-
::::  -::::::::.   `-::::::::  ::::
-::-   .-:::-.``....``.-::-.   -::-
 .. ``       .::::::::.     `..`..
   -:::-`   -::::::::::`  .:::::`
   :::::::` -::::::::::` :::::::.
   .:::::::  -::::::::. ::::::::
    `-:::::`   ..--.`   ::::::.
      `...`  `...--..`  `...`
            .::::::::::
             `.-::::-`
)";
        return parseLogoArt(art);
    }
    if (logoName == "uwuntu" || logoName == "Uwuntu") {
        std::string art =
R"(
                                  &&
                               &&&&&&&&
   ,                  *&&&&&&  &&&&&&&&(
    &%%%%&&&&     &&&&&&&&&&&&  ,&&&&&
     %%$2%%%%&&$1&&&   ,&&&&&&&&&&&&&,   %&&&$&&&%%$%%%.
     &%%%$2%&&&&&$1&&#   &,       &&&&&&$2&&&&&&&%%%$1%%
      &%%&&$2&&&&$1&&&(               &&&$2&&&&&&%$1%%%
       &&&&&$2&&&$1&%                  *&&$2&&&&&$1&&%
    &&&/  &&&&$3\$1&                    ,$3/$1*.**
 %&&&&&&&&  &&&$3⟩$1.,                *.$3⟨$1
 %&&&&&&&&  &&$3/$1..      $3/    \$1      ..$3\$1(&&&&&&
   #&&&#%%%%.%%%(      $3\_/\_/$1      (%%%.%%%%/
        /%%%%%%%&&*              ,&&&%%%%%%&
           &&&&&&&&           &&&&&&&&&&&
            (&&&&&    &&&&&&&&&&&
            $2%%$1  &   &&&&&&&&&&&&  &&&&&&&
           $2%%%$1        #&&&&&&#   &&&&&&&&&
 $2%%%%%     %%$1                     #&&&&&(
$2&%.      %%%$1
  $2%%%%%%%
)";
        return parseLogoArt(art);
    }
if (logoName == "Generatrix Linux" || logoName == "generatrix linux" || logoName == "generatrix") {
       std::string art =
R"(
              -------------#              
          =--------------------           
        -------------------------+        
       =--------------------------=       
      ----------+        =----------      
    @--------=*            =----------    
    --------=                +--------    
   @-------                    -------=   
   =-------       -----+       --------+  
   =-------      =-------       --------  
   -------+       =-------      --------  
    -------+       ------       --------  
    +-------=      ------      +-------=  
     ---------=     --+        --------   
      ------------=--         =--------   
       =-------------        --------=    
         =-----------      ----------     
            =--------  **-----------      
                     ------------=        
                     ----------=*         
                     --------=            
                     -----=               
                                          
)";
         return parseLogoArt(art);
  }
    if (logoName == "small-generatrix") {
          std::string art =
R"(




        ---------=        
     ---------------+     
    -------+--*-------    
   -----+        ------   
  -----            -----  
  -----    ----    %----= 
  ----+     ----    ----- 
  *----=    ---*    ----= 
   +-----=-@-+     -----  
    =--------     -----   
       =-----  =------    
             -------=     
             ------       
             *            
)";
      return parseLogoArt(art);
    } 
    if (logoName == "void" || logoName == "Void") {
        std::string art =
R"(
    _______
 _ \______ -
| \  ___  \ |
| | /   \ | |
| | \___/ | |
| \______ \_|
 -_______\
)";
        return parseLogoArt(art);
    }
    if (logoName == "netbsd" || logoName == "Netbsd" || logoName == "NetBsd") {
        std::string art =
R"(
                    xxxxxxxxxxxxxxxxxxx
                 xxxxxxxxxxxxxxxxxxxxxxxxx
               xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
             xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx:
           xxxxxxxxxxxxxxxxxxxxxxxxxxxxx+;..:+xx
          xxxxxxxxxxxxxxxxxxxxxx:...........+xxxx
           +xxxxxxxxxxxxx+............xxxxxxxxxxxx
         x........................;..............xx
        x.x.............................xxxxxxxxxxx
       :xx.x.......................:xxxxxxxxxxxxxxxx
       xxxx.x...................xxxxxxxxxxxxxxxxxxxx
       xxxx:.x..............xxxxxxxxxxxxxxxxxxxxxxxx
       xxxxx..xxx....:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
       :xxxxx..xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        xxxxxx..xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        ;xxxxxx..xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
         xxxxxxx..xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
          xxxxxxx..xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
           xxxxxx+..xxxxxxxxxxxxxxxxxxxxxxxxxxxx
             xxxxx:..xxxxxxxxxxxxxxxxxxxxxxxxx
               xxxx...xxxxxxxxxxxxxxxxxxxxxx:
                 xxx...xxxxxxxxxxxxxxxxxxx
                    ;...xxxxxxxxxxxxxxx
                           ;xxx;
)";
        return parseLogoArt(art);
    }
if (logoName == "Endeavour" || logoName == "endeavour") {
      std::string art =
  R"(
                    ≈ 
                   √≈≈
                  √≈≈≈∞
                 √√≈≈≈≈√
                 √≈≈≈≈≈≈π
                √∞≈≈≈≈≈≈√√
               π√≈≈≈≈≈≈≈≈√
              √√≈≈≈≈≈≈≈≈≈∞√
             √√∞≈≈≈≈≈≈≈≈≈≈√√
             √√≈≈≈≈≈≈≈≈≈≈≈∞√
            √√≈≈≈≈≈≈≈≈≈≈≈≈≈√π
           √√√≈≈≈≈≈≈≈≈≈≈≈≈≈√√
          √√√≈≈≈≈≈≈≈≈≈≈≈≈≈≈√√
         √√√∞≈≈≈≈≈≈≈≈≈≈≈≈≈≈√√π
         √√√≈≈≈≈≈≈≈≈≈≈≈≈≈≈∞√√√
           √∞∞∞≈≈≈≈≈≈≈≈≈≈∞√√√
          √√√√√√√√√√√√√√√√√√√
          √√√√√√√√√√√√√√ππ
      ╔═╗┌┐┌┌┬┐┌─┐┌─┐┬  ┬┌─┐┬ ┬┬─┐
      ║╣ │││ ││├┤ ├─┤└┐┌┘│ ││ │├┬┘
      ╚═╝┘└┘─┴┘└─┘┴ ┴ └┘ └─┘└─┘┴└─
                             
  )";
  return parseLogoArt(art);
}
    if (logoName == "openbsd") {
        std::string art =
R"(
$3                                     _
                                    (_)
$1              |    .
$1          .   |L  /|   .         $3 _
$1      _ . |\ _| \--+._/| .       $3(_)
$1     / ||\| Y J  )   / |/| ./
    J  |)'( |        ` F`.'/       $3 _
$1  -<|  F         __     .-<        $3(_)
$1    | /       .-'$3. $1`.  /$3-. $1L___
    J \      <    $3\ $1 | | $5O$3\$1|.-' $3 _
$1  _J \  .-    \$3/ $5O $3| $1| \  |$1F    $3(_)
$1 '-F  -<_.     \   .-'  `-' L__
__J  _   _.     >-'  $1)$4._.   $1|-'
$1 `-|.'   /_.          $4\_|  $1 F
  /.-   .                _.<
 /'    /.'             .'  `\
  /L  /'   |/      _.-'-\
 /'J       ___.---'\|
   |\  .--' V  | `. `
   |/`. `-.     `._)
      /.-.\
      \ (  `\
       `.\
)";
        return parseLogoArt(art);
    }
    if (logoName == "FreeBSD" || logoName == "freebsd") {
        std::string art = 
R"(                                                                                                 
```                        $2`
  $1` `.....---...$2....--.```   -/
  $1+o   .--`         $2/y:`      +.
   $1yo`:.            $2:o      `+-
    $1y/               $2-/`   -o/
   $1.-                  $2::/sy+:.
   $1/                     $2`--  /
  $1`:                          $2:`
  $1`:                          $2:`
   $1/                          $2/
   $1.-                        $2-.
    $1--                      $2-.
     $1`:`                  $2`:`
       .--             `--.
          .---.....----.

)";
        return parseLogoArt(art);
    }
    if (logoName == "Unknown" || logoName == "unknown") {
        std::string art =
R"(

       ________
   _jgN########Ngg_
 _N##N@@""  ""9NN##Np_
d###P            N####p
"^^"              T####
                  d###P
               _g###@F
            _gN##@P
          gN###F"
         d###F
        0###F
        0###F
        0###F
        "NN@'

         ___
        q###r
         ""
)";
        return parseLogoArt(art);
    }
    return {};
}
//!!SO MUCH LOGOS FROM FASTFETCH!!

void showLogo(const std::string& color, const std::string& logoName) {
    auto lines = getDifferentLogoLines(logoName);
    if (!lines.empty()) {
        for (const auto& l : lines)
            std::cout << color << l << colors::RESET << "\n";
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

    SysStats info;
    if (!getSysStats(info)) {
        std::cerr << "Error retrieving system information\n";
        return 1;
    }
    struct utsname un;
    uname(&un);
    char hostname[256];
    gethostname(hostname, sizeof hostname);

    std::string distro = getDistro();
    std::string logoName = logoArg.empty() ? "" : logoArg;
    if (logoName.empty()) {
        std::string dl = distro;
        for (auto& c : dl) c = tolower(c);
        if (dl.find("kiss") != std::string::npos)          logoName = "kiss";
        else if (dl.find("alpine") != std::string::npos)   logoName = "alpine";
        else if (dl.find("arch") != std::string::npos)     logoName = "arch";
        else if (dl.find("artix") != std::string::npos)    logoName = "artix";
        else if (dl.find("bedrock") != std::string::npos)  logoName = "bedrock";
        else if (dl.find("chimera") != std::string::npos)  logoName = "chimera";
        else if (dl.find("debian") != std::string::npos)   logoName = "debian";
        else if (dl.find("exherbo") != std::string::npos)  logoName = "exherbo";
        else if (dl.find("gentoo") != std::string::npos)   logoName = "gentoo";
        else if (dl.find("gnu") != std::string::npos)      logoName = "gnu";
        else if (dl.find("guix") != std::string::npos)     logoName = "guix";
        else if (dl.find("haiku") != std::string::npos)    logoName = "haiku";
        else if (dl.find("parabola") != std::string::npos) logoName = "parabola";
        else if (dl.find("raspbian") != std::string::npos) logoName = "raspbian";
        else if (dl.find("uwuntu") != std::string::npos)   logoName = "uwuntu";
        else if (dl.find("void") != std::string::npos)     logoName = "void";
        else if (dl.find("netbsd") != std::string::npos)   logoName = "netbsd";
        else if (dl.find("openbsd") != std::string::npos)  logoName = "openbsd";
        else if (dl.find("freebsd") != std::string::npos)  logoName = "FreeBSD";
    }

    bool useSideBySide = cfg.logo && !logoName.empty() && getDifferentLogoLines(logoName).size() > 0;

    if (useSideBySide) {
        auto logoLines = getDifferentLogoLines(logoName);

        std::vector<std::string> infoLines;
        const size_t labelW = 10;
        auto pad = [](const std::string& s, size_t w) {
            return s.size() < w ? s + std::string(w - s.size(), ' ') : s;
        };
        auto addInfo = [&](const std::string& key, const std::string& val) {
            infoLines.push_back(cfg.textcolor + pad(key, labelW) + colors::RESET + " " + val);
        };

        infoLines.push_back(colors::BOLD + std::string(hostname) + "@" + un.sysname + colors::RESET);
        if (cfg.line) infoLines.push_back("=-=-=-=-=-=-=-=");
        if (cfg.os)         addInfo("os", distro + " " + getArchitecture());
        if (cfg.kernel)     addInfo("kernel", std::string(un.release));
        if (cfg.uptime)     addInfo("uptime", humanUptime(info.uptime));
        if (cfg.usedram)    addInfo("ram", humanBytes(info.totalram - info.freeram) + " / " + humanBytes(info.totalram));
        if (cfg.fullram)    addInfo("totalram", humanBytes(info.totalram));
        if (cfg.procs)      infoLines.push_back(cfg.textcolor + pad("procs", labelW) + colors::RESET + " " + std::to_string(info.procs));
        if (cfg.cpu)        addInfo("cpu", getCPUModel());
        if (cfg.gpu)        addInfo("gpu", getGPUModel());
        if (cfg.shell)      addInfo("shell", getShell());
        if (cfg.terminal)   addInfo("terminal", getTerminal());
        if (cfg.resolution) addInfo("resolution", getResolution());
        if (cfg.packages)   addInfo("packages", getPackages());
        if (cfg.de)         addInfo("de", getDE());
        if (cfg.wm)         addInfo("wm", getWM());
        if (cfg.init)       addInfo("init", getInit());
        if (cfg.battery) { std::string bat = getBattery(); if (!bat.empty()) addInfo("battery", bat); }
        if (cfg.disk)       addInfo("disk", GetDiskInfo());
        if (cfg.lastrun && !cfg.lastrunstr.empty())
            infoLines.push_back(cfg.textcolor + pad("lastrun", labelW) + colors::RESET + " " + cfg.lastrunstr);

        size_t maxLogo = logoLines.size();
        size_t maxInfo = infoLines.size();
        size_t rows = maxLogo > maxInfo ? maxLogo : maxInfo;

        size_t maxLogoWidth = 0;
        for (const auto& l : logoLines) {
            size_t w = visWidth(l);
            if (w > maxLogoWidth) maxLogoWidth = w;
        }

        for (size_t i = 0; i < rows; i++) {
            if (i < maxLogo) {
                if (logoName == "kiss" || logoName == "Kiss")
                    std::cout << logoLines[i];
                else
                    std::cout << cfg.logocolor << logoLines[i] << colors::RESET;
                size_t w = visWidth(logoLines[i]);
                if (w < maxLogoWidth) std::cout << std::string(maxLogoWidth - w, ' ');
            } else {
                std::cout << std::string(maxLogoWidth, ' ');
            }
            std::cout << "  ";
            if (i < maxInfo) std::cout << infoLines[i];
            std::cout << "\n";
        }
        if (cfg.blocks) printBlocks(std::string(maxLogoWidth + 2, ' '), cfg);
    } else {
        if (cfg.logo) showLogo(cfg.logocolor, logoName);
        std::cout << colors::BOLD << hostname << "@" << un.sysname << colors::RESET << "\n";
        if (cfg.line) std::cout << "=-=-=-=-=-=-=-=\n";
        if (cfg.os)         showInfo("os", distro + " " + getArchitecture(), cfg.textcolor);
        if (cfg.kernel)     showInfo("kernel", std::string(un.release), cfg.textcolor);
        if (cfg.uptime)     showInfo("uptime", humanUptime(info.uptime), cfg.textcolor);
        if (cfg.usedram)    showInfo("ram", humanBytes(info.totalram - info.freeram) + " / " + humanBytes(info.totalram), cfg.textcolor);
        if (cfg.fullram)    showInfo("totalram", humanBytes(info.totalram), cfg.textcolor);
        if (cfg.procs)      std::cout << cfg.textcolor << "procs" << "  " << colors::RESET << info.procs << "\n";
        if (cfg.cpu)        showInfo("cpu", getCPUModel(), cfg.textcolor);
        if (cfg.gpu)        showInfo("gpu", getGPUModel(), cfg.textcolor);
        if (cfg.shell)      showInfo("shell", getShell(), cfg.textcolor);
        if (cfg.terminal)   showInfo("terminal", getTerminal(), cfg.textcolor);
        if (cfg.resolution) showInfo("resolution", getResolution(), cfg.textcolor);
        if (cfg.packages)   showInfo("packages", getPackages(), cfg.textcolor);
        if (cfg.de)         showInfo("de", getDE(), cfg.textcolor);
        if (cfg.wm)         showInfo("wm", getWM(), cfg.textcolor);
        if (cfg.init)       showInfo("init", getInit(), cfg.textcolor);
        if (cfg.battery) { std::string bat = getBattery(); if (!bat.empty()) showInfo("battery", bat, cfg.textcolor); }
        if (cfg.disk)       showInfo("disk", GetDiskInfo(), cfg.textcolor);
        if (cfg.lastrun && !cfg.lastrunstr.empty())
            std::cout << cfg.textcolor << "lastrun" << "  " << colors::RESET << cfg.lastrunstr << "\n";
        if (cfg.blocks) printBlocks("  ", cfg);
    }

    try {
        toml::table t = toml::parse_file(path);             
        if (!t.contains("state"))                      
            t.insert("state", toml::table{});
        t["state"].as_table()->insert_or_assign("lastrun", currentTime());
        std::ofstream(path) << t;
    } catch (const toml::parse_error& e) {
        std::cerr << "Error config fail\n";
    }  
} 

