#pragma once

#include "lot_camera.h"
#include "lot_device.h"
#include "lot_frame_info.h"
#include "lot_game_object.h"
#include "lot_pipeline.h"

// std 
#include <memory>
#include <vector>

namespace lot
{
    class PointLightSystem {
        public:
            PointLightSystem(LotDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
            ~PointLightSystem();

            PointLightSystem(const PointLightSystem&) = delete;
            PointLightSystem& operator=(const PointLightSystem&) = delete;

            void update(FrameInfo& frameInfo, GlobalUbo &ubo);
            void render(FrameInfo& frameInfo);
        private:
            void createPipeLineLayout(VkDescriptorSetLayout globalSetLayout);
            void createPipeline(VkRenderPass renderPass);

            LotDevice& lotDevice;

            std::unique_ptr<LotPipeline> lotPipeline;
            VkPipelineLayout pipelineLayout;
    };
} // namespace lot

