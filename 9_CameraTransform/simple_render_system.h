#pragma once

#include "lot_camera.h"
#include "lot_device.h"
#include "lot_game_object.h"
#include "lot_pipeline.h"

// std
#include <memory>
#include <vector>

namespace lot {
    class SimpleRenderSystem {
        public:
            SimpleRenderSystem(LotDevice &device, VkRenderPass renderPass);
            ~SimpleRenderSystem();

            SimpleRenderSystem(const SimpleRenderSystem &) = delete;
            SimpleRenderSystem& operator=(const SimpleRenderSystem &) = delete;

            void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<LotGameObject> &gameObjects, 
                                   const LotCamera& camera);
        private:
            void createPipelineLayout();
            void createPipeline(VkRenderPass renderPass);

            LotDevice& lotDevice;

            std::unique_ptr<LotPipeline> lotPipeline;
            VkPipelineLayout pipelineLayout;
    };
}