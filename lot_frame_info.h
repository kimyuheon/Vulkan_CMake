#pragma once 

#include "lot_camera.h"
#include "lot_game_object.h"

// lib
#include <vulkan/vulkan.h>

namespace lot {
    #define MAX_LIGHTS 10

    struct PointLight {
        glm::vec4 position;
        glm::vec4 color;
    };

    struct GlobalUbo {
        glm::mat4 projection{1.f};
        glm::mat4 View{1.f};
        glm::mat4 inverseView{1.f};
        glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
        PointLight pointLights[MAX_LIGHTS];
        alignas(4) int numLights;
        alignas(4) int lightingEnabled = 1;
    };

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        LotCamera &camera;
        VkDescriptorSet globalDescriptorSet;
        LotGameObject::Map &Objects;
    };
}