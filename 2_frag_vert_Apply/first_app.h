#pragma once

#include "lot_device.h"
#include "lot_model.h"
#include "lot_pipeline.h"
#include "lot_swap_chain.h"
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
            void loadModels();
            void createPipelineLayout();
            void createPipeline();
            void createCommandBuffers();
            void drawFrame();

            LotWindow lotWindow{ WIDTH, HEIGHT, "Hellow Lot Vulkan!!!" };
            LotDevice lotDevice{ lotWindow };
            LotSwapChain lotSwapChain{ lotDevice, lotWindow.getExtent() };

            std::unique_ptr<LotPipeline> lotPipeline;
            VkPipelineLayout pipelineLayout;
            std::vector<VkCommandBuffer> commandBuffers;
            std::unique_ptr<LotModel> lotModel;
    };

} // namespace lot