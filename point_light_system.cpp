#include "point_light_system.h"

//libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//std
#include <array>
#include <cassert>
#include <stdexcept>
#include <map>

namespace lot {
    struct PointLightPushConstants {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius;
    };

    PointLightSystem::PointLightSystem(LotDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
    : lotDevice{device} {
        createPipeLineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    PointLightSystem::~PointLightSystem() {
        vkDestroyPipelineLayout(lotDevice.device(), pipelineLayout, nullptr);
    }

    void PointLightSystem::createPipeLineLayout(VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PointLightPushConstants);
        
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(lotDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void PointLightSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LotPipeline::defaultPipelineConfigInfo(pipelineConfig);
        LotPipeline::enableAlphaBlending(pipelineConfig);
        pipelineConfig.attributeDescription.clear();
        pipelineConfig.bindingDescription.clear();
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        lotPipeline = std::make_unique<LotPipeline>(
            lotDevice, 
            "shaders/point_light.vert.spv", 
            "shaders/point_light.frag.spv" ,
            pipelineConfig);
    }

    void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo &ubo) {
        auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.frameTime, {0.f, -1.f, 0.f});
        int lightIndex = 0;
        for (auto& kv : frameInfo.Objects) {
            auto& obj = kv.second;
            if (obj.pointLight == nullptr) continue;
            
            assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

            obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

            ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
            ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);

            lightIndex++;
        }
        ubo.numLights = lightIndex;
    }

    void PointLightSystem::render(FrameInfo& frameInfo) {
        std::map<float, LotGameObject::id_t> sorted;

        for (auto& kv : frameInfo.Objects) {
            auto& obj = kv.second;
            if (obj.pointLight == nullptr) continue;

            // 카메라로부터의 거리 계산
            auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
            float disSquard = glm::dot(offset, offset);
            sorted[disSquard] = obj.getId();
        }

        lotPipeline->bind(frameInfo.commandBuffer);
        
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 
            1, 
            &frameInfo.globalDescriptorSet, 
            0, 
            nullptr
        );

        for (auto& kv : frameInfo.Objects) {
            auto& obj = kv.second;
            if (obj.pointLight == nullptr) continue;

            PointLightPushConstants push{};
            push.position = glm::vec4(obj.transform.translation, 1.f);
            push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
            push.radius = obj.transform.scale.x;

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PointLightPushConstants),
                &push);

            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        }

    }

}