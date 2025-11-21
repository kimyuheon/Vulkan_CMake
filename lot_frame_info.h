#pragma once 

#include "lot_camera.h"

// lib
#include <vulkan/vulkan.h>

namespace lot {
    struct FrameInfo
    {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        LotCamera &camera;
        VkDescriptorSet globalDescriptorSet;
    };
}