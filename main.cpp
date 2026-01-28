#ifdef _MSC_VER
    #pragma warning(disable: 4819)
#endif

#include "first_app.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
  #include <Windows.h>
#endif

int main() {
  #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
  #endif
    std::setlocale(LC_ALL, "en_US.UTF-8");
  
    lot::FirstApp app{};

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  return EXIT_SUCCESS;
}