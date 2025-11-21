#pragma once

#include "lot_camera.h"
#include "lot_device.h"
#include "lot_frame_info.h"
#include "lot_game_object.h"
#include "lot_pipeline.h"

// std
#include <memory>
#include <vector>

namespace lot {
    class SimpleRenderSystem {
        public:
            SimpleRenderSystem(LotDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
            ~SimpleRenderSystem();

            SimpleRenderSystem(const SimpleRenderSystem &) = delete;
            SimpleRenderSystem& operator=(const SimpleRenderSystem &) = delete;

            void renderGameObjects(FrameInfo &frameInfo, std::vector<LotGameObject> &gameObjects);
            void renderHighlights(FrameInfo &frameInfo, std::vector<LotGameObject> &gameObjects);
        private:
            void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
            void createPipeline(VkRenderPass renderPass);
            void createHighlightPipeline(VkRenderPass renderPass);

            LotDevice& lotDevice;

            std::unique_ptr<LotPipeline> lotPipeline;
            std::unique_ptr<LotPipeline> highlightPipeline;
            VkPipelineLayout pipelineLayout;
    };
}