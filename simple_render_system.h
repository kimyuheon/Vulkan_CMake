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
                                   const LotCamera& camera, bool enableLighting = true);
            void renderHighlights(VkCommandBuffer commandBuffer, std::vector<LotGameObject> &gameObjects,
                                  const LotCamera& camera, bool enableLighting = true);
        private:
            void createPipelineLayout();
            void createPipeline(VkRenderPass renderPass);
            void createHighlightPipeline(VkRenderPass renderPass);

            LotDevice& lotDevice;

            std::unique_ptr<LotPipeline> lotPipeline;
            std::unique_ptr<LotPipeline> highlightPipeline;
            VkPipelineLayout pipelineLayout;
    };
}