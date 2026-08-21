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
        {"pacman", "pacman -Qq 2>/dev/null | wc -l"},
        {"dpkg", "dpkg-query -f '${binary:Package}\\n' -W 2>/dev/null | wc -l"},
        {"rpm", "rpm -qa 2>/dev/null | wc -l"},
        {"zypper", "rpm -qa 2>/dev/null | wc -l"},
        {"apk", "apk info 2>/dev/null | wc -l"},
        {"equery", "equery list 2>/dev/null | wc -l"},
        {"xbps-query", "xbps-query -l 2>/dev/null | wc -l"},
        {"eopkg", "eopkg list-installed 2>/dev/null | wc -l"},
        {"nix-env", "nix-env -q 2>/dev/null | wc -l"}
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

void printBlocks(const std::string& indent = "  ") {
  std::cout << indent;
  for (int i = 0; i < 8; ++i)
    std::cout << "\033[" << (40 + i) << "m" << "   ";
  std::cout << colors::RESET << "\n" << indent;
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

static std::vector<std::string> parseLogoArt(const std::string& art, const std::string(&palette)[5], const std::string& def) {
    std::vector<std::string> lines;
    std::istringstream ss(art);
    std::string raw;
    while (std::getline(ss, raw)) {
        std::string line;
        std::string cur = def;
        for (size_t i = 0; i < raw.size(); i++) {
            if (raw[i] == '$' && i + 1 < raw.size() && raw[i + 1] >= '1' && raw[i + 1] <= '4') {
                cur = palette[raw[i + 1] - '0'];
                i++;
            } else if (raw[i] == ' ') {
                line += "  ";
            } else {
                line += cur + "██" + "\033[0m";
            }
        }
        lines.push_back("  " + line);
    }
    return lines;
}

std::vector<std::string> getDifferentLogoLines(const std::string& logoName) {
    std::string def;

    if (logoName == "kiss" || logoName == "Kiss") {
        const std::string R  = "\033[38;2;255;0;0m██";       
        const std::string W  = "\033[38;2;255;255;255m██";   
        const std::string P  = "\033[38;2;210;0;70m██";     
        const std::string S  = "  ";                        
        const std::string RS = "\033[0m";                    

        const std::vector<std::string> logo = {
            S+S+S+S+S+R+R+R+R+S+S+S+R+R+R+R+S+S+S+S+S,
            S+S+S+W+W+R+R+R+R+R+R+R+R+R+R+R+R+S+S+S+S,
            S+S+P+W+W+R+R+R+R+R+R+R+R+R+R+R+R+R+S+S+S,
            S+R+W+R+R+W+W+W+W+W+W+W+W+W+W+R+R+R+R+S+S,
            R+R+R+R+W+W+W+W+W+W+W+W+W+W+W+W+R+R+R+R+S,
            R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+S,
            S+R+R+R+R+R+R+W+W+W+W+W+R+R+R+R+R+R+R+S+S,
            S+S+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+R+S+S+S,
            S+S+S+S+S+R+R+R+R+R+R+R+R+R+R+S+S+S+S+S+S
        };

        std::vector<std::string> lines;
        lines.reserve(logo.size());
        for (const auto& row : logo) {
            lines.push_back("  " + row + RS);
        }
        return lines;
    }
    if (logoName == "alpine") {
        std::string art =
R"(
    /\ /\
   / / \ \
  / /   \ \
 / /     \ \
         \
)";
        std::string palette[5] = {};
        def = "\033[38;2;13;183;216m";
        return parseLogoArt(art, palette, def);
    }
    if (logoName == "alpine2") {
        std::string art =
R"(
$1   /\ /\
  /$2/ $1\  \
 /$2//  $1\  \
/$2//    $1\  \
$2//      $1\  \
         \
)";
        std::string palette[5] = {};
        palette[1] = "\033[38;2;255;255;255m";
        palette[2] = "\033[38;2;13;183;216m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        palette[2] = "\033[38;2;23;147;209m";
        def = "\033[38;2;23;147;209m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        def = "\033[38;2;116;194;210m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        def = "\033[38;2;116;194;210m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        palette[1] = "\033[38;2;255;255;255m";
        palette[2] = "\033[38;2;90;90;90m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        palette[1] = "\033[38;2;255;255;255m";
        palette[2] = "\033[38;2;15;52;96m";
        palette[3] = "\033[38;2;215;0;65m";
        palette[4] = "\033[38;2;47;197;92m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        palette[1] = "\033[38;2;215;0;65m";
        palette[2] = "\033[38;2;15;52;96m";
        palette[3] = "\033[38;2;255;255;255m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        def = "\033[38;2;215;0;65m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        palette[1] = "\033[38;2;255;100;0m";
        palette[2] = "\033[38;2;255;255;255m";
        palette[3] = "\033[38;2;140;140;140m";
        def = "\033[38;2;255;100;0m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        palette[2] = "\033[38;2;150;72;210m";
        def = "\033[38;2;150;72;210m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        def = "\033[38;2;90;90;90m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        def = "\033[38;2;100;50;200m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        def = "\033[38;2;215;183;0m";
        return parseLogoArt(art, palette, def);
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
        std::string palette[5] = {};
        def = "\033[38;2;90;135;255m";
        return parseLogoArt(art, palette, def);
    }
    if (logoName == "raspbian") {
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
        std::string palette[5] = {};
        palette[1] = "\033[38;2;47;197;92m";
        palette[2] = "\033[38;2;190;30;30m";
        return parseLogoArt(art, palette, def);
    }
    if (logoName == "uwuntu") {
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
        std::string palette[5] = {};
        palette[1] = "\033[38;2;255;220;0m";
        palette[2] = "\033[38;2;47;197;92m";
        palette[3] = "\033[38;2;0;200;255m";
        def = "\033[38;2;255;220;0m";
        return parseLogoArt(art, palette, def);
    }
    if (logoName == "void") {
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
        std::string palette[5] = {};
        palette[1] = "\033[38;2;47;197;92m";
        palette[2] = "\033[38;2;47;197;92m";
        def = "\033[38;2;47;197;92m";
        return parseLogoArt(art, palette, def);
    }
    if (logoName == "unknown") {
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
        std::string palette[5] = {};
        def = "\033[38;2;180;180;180m";
        return parseLogoArt(art, palette, def);
    }
    return {};
}

void showLogo(const std::string& color, const std::string& logoName) {
    auto lines = getDifferentLogoLines(logoName);
    if (!lines.empty()) {
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
    }

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
    if (cfg.disk)       showInfo("disk", GetDiskInfo(), cfg.textcolor);
    if (cfg.lastrun && !cfg.lastrunstr.empty())
        std::cout << cfg.textcolor << "lastrun" << "  " << colors::RESET << cfg.lastrunstr << "\n";
    if (cfg.blocks) printBlocks();

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
