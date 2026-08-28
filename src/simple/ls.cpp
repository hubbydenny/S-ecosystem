/// #TODO  человекочитаемые размеры (KiB/MiB)
#include <iostream>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, char* argv[]) {

  std::string targetdirect = (argc > 1) ? argv[1] : ".";
  std::vector<std::string> names;
  try {
    for (const auto& entry : std::filesystem::directory_iterator(targetdirect)) {
      std::string filename = entry.path().filename().string();
      if (filename.rfind(".", 0) == 0) continue;
      names.push_back(filename);
    }
  }
  catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "err <<" << e.what() << std::endl;
    return 1;
  }

  if (names.empty()) return 0;

  if (names.size() <= 10) {
    for (size_t i = 0; i < names.size(); i++) {
      if (i) std::cout << "  ";
      std::cout << names[i];
    }
    std::cout << std::endl;
    return 0;
  }

  size_t maxlen = 0;
  for (const auto& n : names) if (n.size() > maxlen) maxlen = n.size();
  size_t colw = maxlen + 2;

  int tw = 80;
  if (isatty(STDOUT_FILENO)) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) tw = ws.ws_col;
  }
  size_t cols = tw / colw;
  if (cols < 1) cols = 1;
  size_t rows = (names.size() + cols - 1) / cols;

  for (size_t r = 0; r < rows; r++) {
    for (size_t c = 0; c < cols; c++) {
      size_t idx = c * rows + r;
      if (idx >= names.size()) break;
      std::cout << names[idx];
      if (c + 1 < cols) std::cout << std::string(colw - names[idx].size(), ' ');
    }
    std::cout << std::endl;
  }
  return 0;
}
