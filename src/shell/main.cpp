#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <climits>
#include <map>
#include <fstream>
#include <toml++/toml.hpp>
#include <termios.h>
#include <fcntl.h>
#include <cctype>
#include <filesystem>

static std::map<std::string, std::string> g_aliases;
static std::string g_histfile;
static int g_last_status = 0;

static std::vector<std::string> g_history;
static size_t g_histidx = 0;

struct TermRaw {
    termios old;
    bool ok = false;
    TermRaw() {
        if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &old) == 0) {
            termios raw = old;
            raw.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            ok = true;
        }
    }
    ~TermRaw() { if (ok) tcsetattr(STDIN_FILENO, TCSANOW, &old); }
};

static std::string make_prompt() {
    char cwd[4096];
    if (!getcwd(cwd, sizeof cwd)) std::strcpy(cwd, "?");
    std::string path = cwd;
    const char* home = getenv("HOME");
    if (home && path.compare(0, std::strlen(home), home) == 0)
        path = "~" + path.substr(std::strlen(home));
    const char* user = getenv("USER");
    if (!user) user = "?";
    return std::string(user) + ":" + path + " se> ";
}

static void refresh_line(const std::string& prompt, const std::string& line, size_t pos) {
    std::cout << "\r\x1b[K" << prompt << line;
    size_t back = line.size() - pos;
    if (back) std::cout << "\x1b[" << back << "D";
    std::cout << std::flush;
}

static std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

static std::string self_dir() {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    if (n > 0) {
        buf[n] = 0;
        std::string p(buf);
        size_t slash = p.find_last_of('/');
        if (slash != std::string::npos) return p.substr(0, slash);
    }
    return ".";
}

static int builtin_cd(const std::vector<std::string>& args) {
    const char* target = args.size() > 1 ? args[1].c_str() : getenv("HOME");
    if (!target) target = "/";
    if (chdir(target) != 0) {
        std::cerr << "cd: " << target << ": " << strerror(errno) << "\n";
        return 1;
    }
    return 0;
}

static void show_help() {
    std::cout <<
        "S-ecosystem shell (se)\n"
        "builtins:\n"
        "  help              show this help\n"
        "  cd [dir]          change directory\n"
        "  pwd               print working directory\n"
        "  echo [text]       print text\n"
        "  clear             clear screen\n"
        "  exit | quit       leave the shell\n"
        "  alias [n=v]       show/define aliases (always work at runtime)\n"
        "  unalias <name>    remove an alias\n"
        "  commit [msg]      git add -A && git commit -m [msg]\n"
        "S-ecosystem commands (run as programs):\n"
        "  sfetch            system info fetch\n"
        "  scat <file>       print file (alias: cat)\n"
        "  sls [dir]         list directory (alias: ls)\n"
        "  sk <pid>          kill process by pid\n"
        "  scopy <from> <to> copy file or directory\n"
        "  space [path]      show disk space\n"
        "  srm <path>        remove file or directory\n"
        "  smv <from> <to>   rename/move\n"
        "operators: | (pipe)  > >> < (redirect)  ; && || (chains)  \"...\" '...' (quotes)\n"
        "aliases: ~/.config/se/config.toml  ([aliases] name = \"command\")\n";
}

static int run_external(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) { std::cerr << "fork failed\n"; return 1; }
    if (pid == 0) {
        execvp(argv[0], argv.data());
        std::cerr << args[0] << ": command not found\n";
        std::exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}

static bool run_builtin(const std::vector<std::string>& args) {
    const std::string& cmd = args[0];
    if (cmd == "help") { show_help(); return true; }
    if (cmd == "exit" || cmd == "quit") { std::exit(0); }
    if (cmd == "cd")   { builtin_cd(args); return true; }
    if (cmd == "pwd")  { char cwd[4096]; if (getcwd(cwd, sizeof cwd)) std::cout << cwd << "\n"; return true; }
    if (cmd == "echo") {
        for (size_t i = 1; i < args.size(); i++)
            std::cout << (i > 1 ? " " : "") << args[i];
        std::cout << "\n";
        return true;
    }
    if (cmd == "clear") { std::cout << "\033[2J\033[H"; return true; }
    if (cmd == "alias") {
        if (args.size() == 1) {
            for (auto& [k, v] : g_aliases)
                std::cout << k << "='" << v << "'\n";
            return true;
        }
        for (size_t i = 1; i < args.size(); i++) {
            auto eq = args[i].find('=');
            if (eq == std::string::npos) {
                auto it = g_aliases.find(args[i]);
                if (it != g_aliases.end()) std::cout << it->first << "='" << it->second << "'\n";
                else std::cerr << "alias: " << args[i] << ": not found\n";
                continue;
            }
            g_aliases[args[i].substr(0, eq)] = args[i].substr(eq + 1);
        }
        return true;
    }
    if (cmd == "unalias") {
        for (size_t i = 1; i < args.size(); i++) {
            if (g_aliases.erase(args[i]) == 0)
                std::cerr << "unalias: " << args[i] << ": not found\n";
        }
        return true;
    }
    if (cmd == "commit") {
        std::string msg = "update";
        if (args.size() > 1) {
            msg = args[1];
            for (size_t i = 2; i < args.size(); i++) msg += " " + args[i];
        }
        if (run_external({"git", "add", "-A"}) != 0) {
            std::cerr << "commit: git add failed\n";
            return true;
        }
        if (run_external({"git", "commit", "-m", msg}) != 0) {
            std::cerr << "commit: nothing to commit or git error\n";
            return true;
        }
        std::cout << "committed: " << msg << "\n";
        return true;
    }
    return false;
}

static void load_aliases(const std::string& path) {
    std::ifstream f(path);
    if (!f) return;
    try {
        toml::table t = toml::parse(f, path);
        if (auto* sec = t.get("aliases")) {
            if (auto* tab = sec->as_table()) {
                for (auto& [k, v] : *tab)
                    if (auto s = v.value<std::string>())
                        g_aliases[std::string(k)] = *s;
            }
        }
    } catch (...) {}
}

static std::vector<std::string> expand_alias(std::vector<std::string> args) {
    for (int d = 0; d < 16 && !args.empty(); d++) {
        auto it = g_aliases.find(args[0]);
        if (it == g_aliases.end()) break;
        auto pre = tokenize(it->second);
        pre.insert(pre.end(), args.begin() + 1, args.end());
        args = std::move(pre);
    }
    return args;
}

static std::string expand_env_word(const std::string& w, int st) {
    std::string out; size_t i = 0;
    while (i < w.size()) {
        if (w[i] == '$') {
            if (i + 1 < w.size() && w[i+1] == '?') { out += std::to_string(st); i += 2; continue; }
            size_t j = i + 1;
            if (i + 1 < w.size() && w[i+1] == '{') {
                size_t k = w.find('}', i + 2);
                if (k != std::string::npos) {
                    std::string name = w.substr(i + 2, k - (i + 2));
                    const char* v = std::getenv(name.c_str());
                    out += v ? v : "";
                    i = k + 1; continue;
                }
            }
            while (j < w.size() && (std::isalnum((unsigned char)w[j]) || w[j] == '_')) j++;
            if (j > i + 1) {
                std::string name = w.substr(i + 1, j - (i + 1));
                const char* v = std::getenv(name.c_str());
                out += v ? v : "";
                i = j; continue;
            }
            out += '$'; i++;
        } else { out += w[i]; i++; }
    }
    return out;
}

static std::vector<std::string> expand_env_all(std::vector<std::string> args) {
    for (auto& a : args) a = expand_env_word(a, g_last_status);
    return args;
}

struct Redirect { std::string in, out; bool append = false; };
struct Command  { std::vector<std::string> argv; Redirect redir; };
struct Pipeline { std::vector<Command> cmds; };
struct ChainItem { Pipeline pipe; enum Op { SEQ, AND, OR } op = SEQ; };

enum TokT { T_WORD, T_PIPE, T_SEMI, T_AND, T_OR, T_IN, T_OUT, T_APPEND };
struct Token { TokT t; std::string s; };

static std::vector<Token> tokenize_line(const std::string& line) {
    std::vector<Token> toks;
    std::string cur; bool has = false; size_t i = 0;
    auto flush = [&]() { if (has) { toks.push_back({T_WORD, cur}); cur.clear(); has = false; } };
    while (i < line.size()) {
        char c = line[i];
        if (c == '\'' || c == '"') {
            char q = c; i++; std::string lit;
            while (i < line.size() && line[i] != q) {
                if (q == '"' && line[i] == '\\' && i + 1 < line.size()) { i++; lit.push_back(line[i]); i++; }
                else { lit.push_back(line[i]); i++; }
            }
            i++; cur += lit; has = true; continue;
        }
        if (c == ' ' || c == '\t') { flush(); i++; continue; }
        if (c == '|') { flush(); if (i + 1 < line.size() && line[i+1] == '|') { toks.push_back({T_OR,""}); i += 2; } else { toks.push_back({T_PIPE,""}); i++; } continue; }
        if (c == '&') { flush(); if (i + 1 < line.size() && line[i+1] == '&') { toks.push_back({T_AND,""}); i += 2; } continue; }
        if (c == ';') { flush(); toks.push_back({T_SEMI,""}); i++; continue; }
        if (c == '<') { flush(); toks.push_back({T_IN,""}); i++; continue; }
        if (c == '>') { flush(); if (i + 1 < line.size() && line[i+1] == '>') { toks.push_back({T_APPEND,""}); i += 2; } else { toks.push_back({T_OUT,""}); i++; } continue; }
        cur.push_back(c); has = true; i++;
    }
    flush();
    return toks;
}

static std::vector<ChainItem> parse(const std::string& line) {
    auto toks = tokenize_line(line);
    std::vector<ChainItem> items;
    Pipeline curPipe; Command curCmd; int pending = 0; ChainItem::Op lastOp = ChainItem::SEQ;
    auto pushCmd = [&]() { curPipe.cmds.push_back(curCmd); curCmd = Command{}; };
    auto pushPipe = [&]() { pushCmd(); items.push_back({curPipe, lastOp}); curPipe = Pipeline{}; };
    for (auto& tk : toks) {
        if (tk.t == T_WORD) {
            if (pending) {
                if (pending == 1) curCmd.redir.in = tk.s;
                else { curCmd.redir.out = tk.s; if (pending == 3) curCmd.redir.append = true; }
                pending = 0;
            } else curCmd.argv.push_back(tk.s);
        } else if (tk.t == T_PIPE) pushCmd();
        else if (tk.t == T_IN) pending = 1;
        else if (tk.t == T_OUT) pending = 2;
        else if (tk.t == T_APPEND) pending = 3;
        else if (tk.t == T_SEMI) { pushPipe(); lastOp = ChainItem::SEQ; }
        else if (tk.t == T_AND)  { pushPipe(); lastOp = ChainItem::AND; }
        else if (tk.t == T_OR)   { pushPipe(); lastOp = ChainItem::OR; }
    }
    pushPipe();
    return items;
}

static std::vector<char*> vec_cstr(std::vector<std::string>& v) {
    std::vector<char*> r;
    for (auto& s : v) r.push_back(const_cast<char*>(s.c_str()));
    r.push_back(nullptr);
    return r;
}

static int run_pipeline(const Pipeline& p) {
    int prev_read = -1;
    int status = 0;
    std::vector<pid_t> pids;
    for (size_t i = 0; i < p.cmds.size(); i++) {
        const Command& cmd = p.cmds[i];
        int pipefd[2] = { -1, -1 };
        if (i + 1 < p.cmds.size()) pipe(pipefd);
        pid_t pid = fork();
        if (pid == 0) {
            if (i == 0 && !cmd.redir.in.empty()) {
                int fd = open(cmd.redir.in.c_str(), O_RDONLY);
                if (fd < 0) { std::cerr << "se: cannot open " << cmd.redir.in << "\n"; _exit(1); }
                dup2(fd, 0); close(fd);
            } else if (prev_read != -1) dup2(prev_read, 0);
            if (i == p.cmds.size() - 1 && !cmd.redir.out.empty()) {
                int flags = O_WRONLY | O_CREAT | (cmd.redir.append ? O_APPEND : O_TRUNC);
                int fd = open(cmd.redir.out.c_str(), flags, 0644);
                if (fd < 0) { std::cerr << "se: cannot open " << cmd.redir.out << "\n"; _exit(1); }
                dup2(fd, 1); close(fd);
            } else if (pipefd[1] != -1) dup2(pipefd[1], 1);
            if (prev_read != -1) close(prev_read);
            if (pipefd[0] != -1) close(pipefd[0]);
            if (pipefd[1] != -1) close(pipefd[1]);
            auto args = expand_env_all(expand_alias(cmd.argv));
            if (!args.empty()) {
                if (run_builtin(args)) { std::cout << std::flush; _exit(0); }
                execvp(args[0].c_str(), vec_cstr(args).data());
                std::cerr << args[0] << ": command not found\n";
            }
            _exit(args.empty() ? 0 : 127);
        }
        if (prev_read != -1) close(prev_read);
        if (pipefd[1] != -1) close(pipefd[1]);
        prev_read = pipefd[0];
        pids.push_back(pid);
    }
    for (auto pid : pids) waitpid(pid, &status, 0);
    if (prev_read != -1) close(prev_read);
    return WEXITSTATUS(status);
}

static int run_item(const Pipeline& p) {
    if (p.cmds.size() == 1 && p.cmds[0].redir.in.empty() && p.cmds[0].redir.out.empty()) {
        const Command& c = p.cmds[0];
        if (c.argv.empty()) return 0;
        auto args = expand_env_all(expand_alias(c.argv));
        if (run_builtin(args)) return 0;
        return run_external(args);
    }
    return run_pipeline(p);
}

static void execute(const std::string& line) {
    auto items = parse(line);
    int status = 0;
    for (size_t i = 0; i < items.size(); i++) {
        bool run = true;
        if (i > 0) {
            auto op = items[i-1].op;
            if (op == ChainItem::AND && status != 0) run = false;
            else if (op == ChainItem::OR && status == 0) run = false;
        }
        if (run) status = run_item(items[i].pipe);
        g_last_status = status;
    }
}

static bool read_line(std::string& out) {
    if (!isatty(STDIN_FILENO))
        return static_cast<bool>(std::getline(std::cin, out));
    TermRaw tr;
    std::string prompt = make_prompt();
    std::cout << prompt << std::flush;
    std::string line;
    size_t pos = 0;
    g_histidx = g_history.size();
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        unsigned char uc = (unsigned char)c;
        if (c == '\n' || c == '\r') {
            std::cout << "\n";
            out = line;
            return true;
        } else if (c == 0x03) {
            std::cout << "^C\n";
            line.clear(); pos = 0; g_histidx = g_history.size();
            prompt = make_prompt();
            std::cout << prompt << std::flush;
        } else if (c == 0x04) {
            if (line.empty()) { std::cout << "\n"; return false; }
        } else if (c == 0x7f || c == '\b') {
            if (pos > 0) { line.erase(pos - 1, 1); pos--; refresh_line(prompt, line, pos); }
        } else if (c == 0x1b) {
            char b1 = 0, b2 = 0, b3 = 0;
            if (read(STDIN_FILENO, &b1, 1) != 1) continue;
            if (b1 == '[') {
                if (read(STDIN_FILENO, &b2, 1) != 1) continue;
                if (b2 >= '0' && b2 <= '9') {
                    read(STDIN_FILENO, &b3, 1);
                    if (b2 == '1' && b3 == '~') { pos = 0; refresh_line(prompt, line, pos); }
                    else if (b2 == '4' && b3 == '~') { pos = line.size(); refresh_line(prompt, line, pos); }
                    else if (b2 == '3' && b3 == '~') { if (pos < line.size()) line.erase(pos, 1); refresh_line(prompt, line, pos); }
                } else if (b2 == 'A') {
                    if (g_histidx > 0) { g_histidx--; line = g_history[g_histidx]; pos = line.size(); refresh_line(prompt, line, pos); }
                } else if (b2 == 'B') {
                    if (g_histidx < g_history.size()) g_histidx++;
                    line = (g_histidx == g_history.size() ? std::string() : g_history[g_histidx]);
                    pos = line.size(); refresh_line(prompt, line, pos);
                } else if (b2 == 'C') {
                    if (pos < line.size()) { pos++; refresh_line(prompt, line, pos); }
                } else if (b2 == 'D') {
                    if (pos > 0) { pos--; refresh_line(prompt, line, pos); }
                }
            } else if (b1 == 'O') {
                if (read(STDIN_FILENO, &b2, 1) == 1) {
                    if (b2 == 'H') { pos = 0; refresh_line(prompt, line, pos); }
                    else if (b2 == 'F') { pos = line.size(); refresh_line(prompt, line, pos); }
                }
            }
        } else if (uc >= 0x20) {
            line.insert(pos, 1, c);
            pos++;
            refresh_line(prompt, line, pos);
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    std::string dir = self_dir();
    std::string path = getenv("PATH") ? getenv("PATH") : "";
    setenv("PATH", (dir + ":" + path).c_str(), 1);

    std::string cfgpath = std::string(getenv("HOME")) + "/.config/se/config.toml";
    load_aliases(cfgpath);

    if (argc >= 3 && std::string(argv[1]) == "-c") {
        execute(argv[2]);
        return 0;
    }

    std::string line;
    {
        std::error_code ec;
        std::filesystem::create_directories(std::string(std::getenv("HOME")) + "/.cache/se", ec);
        g_histfile = std::string(std::getenv("HOME")) + "/.cache/se/history";
        std::ifstream hf(g_histfile);
        std::string hl;
        while (std::getline(hf, hl)) if (!hl.empty()) g_history.push_back(hl);
    }

    while (true) {
        if (!read_line(line)) break;
        if (line.empty()) continue;
        g_history.push_back(line);
        {
            std::ofstream out(g_histfile, std::ios::app);
            if (out) out << line << "\n";
        }
        execute(line);
    }
    return 0;
}
