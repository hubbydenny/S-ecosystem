#pragma once
#include <cstdio>
#include <string>
namespace colors {
  const char* BLACK = "\033[0;30m";
  const char* RED = "\033[0;31m";
  const char* GREEN = "\033[0;32m";
  const char* YELLOW = "\033[0;33m";
  const char* BLUE = "\033[0;34m";
  const char* MAGENTA = "\033[0;35m";
  const char* CYAN = "\033[0;36m";
  const char* WHITE = "\033[0;37m";

  const char* BRIGHT_BLACK = "\033[1;30m";
  const char* BRIGHT_RED = "\033[1;31m";
  const char* BRIGHT_GREEN = "\033[1;32m";
  const char* BRIGHT_YELLOW = "\033[1;33m";
  const char* BRIGHT_BLUE = "\033[1;34m";
  const char* BRIGHT_MAGENTA = "\033[1;35m";
  const char* BRIGHT_CYAN = "\033[1;36m";
  const char* BRIGHT_WHITE = "\033[1;37m";

  const char* BG_BLACK = "\033[40m";
  const char* BG_RED = "\033[41m";
  const char* BG_GREEN = "\033[42m";
  const char* BG_YELLOW = "\033[43m";
  const char* BG_BLUE = "\033[44m";
  const char* BG_MAGENTA = "\033[45m";
  const char* BG_CYAN = "\033[46m";
  const char* BG_WHITE = "\033[47m";
  const char* BG_BRIGHT_BLACK = "\033[100m";
  const char* BG_BRIGHT_RED = "\033[101m";
  const char* BG_BRIGHT_GREEN = "\033[102m";
  const char* BG_BRIGHT_YELLOW = "\033[103m";
  const char* BG_BRIGHT_BLUE = "\033[104m";
  const char* BG_BRIGHT_MAGENTA = "\033[105m";
  const char* BG_BRIGHT_CYAN = "\033[106m";
  const char* BG_BRIGHT_WHITE = "\033[107m";

  const char* RESET = "\033[0m";
  const char* BOLD = "\033[1m";
  const char* DIM = "\033[2m";
  const char* ITALIC = "\033[3m";
  const char* UNDERLINE = "\033[4m";
  const char* BLINK = "\033[5m";
  const char* REVERSE = "\033[7m";
  const char* HIDDEN = "\033[8m";
  const char* STRIKETHROUGH = "\033[9m";

  const char* GRAY_1 = "\033[38;5;8m";
  const char* GRAY_2 = "\033[38;5;59m";
  const char* GRAY_3 = "\033[38;5;102m";
  const char* GRAY_4 = "\033[38;5;145m";
  const char* GRAY_5 = "\033[38;5;188m";

  const char* ORANGE = "\033[38;5;214m";
  const char* DARK_ORANGE = "\033[38;5;208m";
  const char* LIGHT_ORANGE = "\033[38;5;220m";

  const char* PINK = "\033[38;5;219m";
  const char* LIGHT_PINK = "\033[38;5;225m";
  const char* HOT_PINK = "\033[38;5;198m";

  const char* PURPLE = "\033[38;5;135m";
  const char* DARK_PURPLE = "\033[38;5;55m";
  const char* LIGHT_PURPLE = "\033[38;5;177m";

  const char* LIME = "\033[38;5;82m";
  const char* SEA_GREEN = "\033[38;5;37m";
  const char* OLIVE = "\033[38;5;100m";

  const char* LIGHT_BLUE = "\033[38;5;39m";
  const char* DARK_BLUE = "\033[38;5;18m";
  const char* SKY_BLUE = "\033[38;5;117m";
  const char* DARK_RED = "\033[38;5;52m";
  const char* CRIMSON = "\033[38;5;124m";
  const char* SALMON = "\033[38;5;209m";

  const char* BROWN = "\033[38;5;94m";
  const char* DARK_BROWN = "\033[38;5;58m";

  const char* CHOCOLATE = "\033[38;5;130m";
  const char* TURQUOISE = "\033[38;5;45m";
  const char* AQUA = "\033[38;5;51m";
  const char* TEAL = "\033[38;5;6m";
  const char* GOLD = "\033[38;5;226m";
  const char* SILVER = "\033[38;5;250m";

  // 24-bit truecolor foreground escape (fastfetch-style)
  inline std::string TRUECOLOR(int r, int g, int b) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "\033[38;2;%d;%d;%dm", r, g, b);
    return std::string(buf);
  }
}
