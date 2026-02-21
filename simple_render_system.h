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
            SimpleRenderSystem(LotDevice &device, VkRenderPass renderPass, 
                               VkDescriptorSetLayout globalSetLayout,
                               VkDescriptorSetLayout textureSetLayout);
            ~SimpleRenderSystem();

            SimpleRenderSystem(const SimpleRenderSystem &) = delete;
            SimpleRenderSystem& operator=(const SimpleRenderSystem &) = delete;

            void renderGameObjects(FrameInfo &frameInfo);
            void renderHighlights(FrameInfo &frameInfo);
        private:
            void createPipelineLayout(VkDescriptorSetLayout globalSetLayout,
                                      VkDescriptorSetLayout textureSetLayout);
            void createPipeline(VkRenderPass renderPass);
            void createHighlightPipeline(VkRenderPass renderPass);

            LotDevice& lotDevice;

            std::unique_ptr<LotPipeline> lotPipeline;
            std::unique_ptr<LotPipeline> highlightPipeline;
            VkPipelineLayout pipelineLayout;
    };
}