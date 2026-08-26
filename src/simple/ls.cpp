/// #TODO  человекочитаемые размеры (KiB/MiB)
#include <iostream>
#include <filesystem>
#include <ostream>
#include <string>

int main(int argc, char* argv[]) {

  std::string targetdirect = (argc > 1) ? argv[1] : ".";
  try {
  for (const auto& entry : std::filesystem::directory_iterator(targetdirect)) { 
  std::string filename = entry.path().filename().string();
 if (filename.rfind(".", 0) == 0) {
                continue;
       }
  std::cout << filename.c_str() << "  "; 
    }
  std::cout << std::endl;
  }
  catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "err <<" << e.what() << std::endl;
    return 1;
  }
  return 0;
}

