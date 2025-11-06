#ifdef _MSC_VER
    #pragma warning(disable: 4819)
#endif

#include "first_app.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
  lot::FirstApp app{};

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  return EXIT_SUCCESS;
}