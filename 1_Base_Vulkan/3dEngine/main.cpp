#include "lot_app_base.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
  lot::LotAppBase app{};

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  return EXIT_SUCCESS;
}
