#pragma once

#include "lot_window.h"

namespace lot {
    class LotAppBase {
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;

            void run();
        private:
            LotWindow lotWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    };
}