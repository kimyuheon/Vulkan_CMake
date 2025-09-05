#pragma once

#include "lot_device.h"
#include "lot_game_object.h"
#include "lot_renderer.h"
#include "lot_window.h"

#include <memory>
#include <vector>

namespace lot {
    class FirstApp {
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;

            FirstApp();
            ~FirstApp();

            FirstApp(const FirstApp &) = delete;
            FirstApp &operator=(const FirstApp&) = delete;

            void run();

        private:
            void loadGameObjects();

            LotWindow lotWindow{ WIDTH, HEIGHT, "Hellow Lot Vulkan!!!" };
            LotDevice lotDevice{ lotWindow };
            LotRenderer lotRenderer{ lotWindow, lotDevice };
            
            std::vector<LotGameObject> gameObjects;
    };

} // namespace lot