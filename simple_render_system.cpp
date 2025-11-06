#include "simple_render_system.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace lot {
    struct SimplePushConstantData {
        //glm::mat4 transform{1.f};
        glm::mat4 modelMatrix{1.f};
        glm::mat4 normalMatrix{1.f};
        alignas(16) glm::vec3 color;
        alignas(4) int isSelected{0};       // 선택 상태 플래그 추가
        alignas(4) int enableLighting{1};   // 조명 활성화 플래그 추가
    };
    

    SimpleRenderSystem::SimpleRenderSystem(LotDevice &device, VkRenderPass renderPass)
    : lotDevice{device}  {
        createPipelineLayout();
        createPipeline(renderPass);
        createHighlightPipeline(renderPass);
    }

    SimpleRenderSystem::~SimpleRenderSystem() {
        vkDeviceWaitIdle(lotDevice.device());
        vkDestroyPipelineLayout(lotDevice.device(), pipelineLayout, nullptr);
    }

    void SimpleRenderSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(lotDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LotPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        lotPipeline = std::make_unique<LotPipeline>(
            lotDevice, 
            "shaders/simple_shader.vert.spv", 
            "shaders/simple_shader.frag.spv" ,
            pipelineConfig);
    }

    void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, 
                                               std::vector<LotGameObject> &gameObjects, 
                                               const LotCamera& camera, bool enableLighting) {
        lotPipeline->bind(commandBuffer);
        
        auto projectionView = camera.getProjection() * camera.getView();

        for (auto& obj : gameObjects) {
            //obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.0001f, glm::two_pi<float>());
            //obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.00005f, glm::two_pi<float>());

            SimplePushConstantData push{};
            //push.color = obj.color;
            //push.transform = projectionView * obj.transform.mat4();
            push.modelMatrix = projectionView * obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();
            push.isSelected = obj.isSelected ? 1 : 0;  // 선택 상태 전달
            push.enableLighting = enableLighting ? 1 : 0;  // 조명 플래그 전달

            vkCmdPushConstants(
                commandBuffer, pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(SimplePushConstantData), &push);

            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }

    void SimpleRenderSystem::createHighlightPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create highlight pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LotPipeline::defaultPipelineConfigInfo(pipelineConfig);

        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        #ifdef __APPLE__
        pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        pipelineConfig.rasterizationInfo.lineWidth = 1.0f;
        #else
        pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
        pipelineConfig.rasterizationInfo.lineWidth = 3.0f;
        #endif

        highlightPipeline = std::make_unique<LotPipeline>(
            lotDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig);
    }

    void SimpleRenderSystem::renderHighlights(VkCommandBuffer commandBuffer,
                                              std::vector<LotGameObject> &gameObjects,
                                              const LotCamera& camera, bool enableLighting) {
        highlightPipeline->bind(commandBuffer);

        auto projectionView = camera.getProjection() * camera.getView();

        int selectedCount = 0;
        for (auto& obj : gameObjects) {
            if (!obj.isSelected) continue;

            selectedCount++;

            SimplePushConstantData push{};
            //push.color = glm::vec3(1.0f, 0.5f, 0.0f);  // 주황색 하이라이트
            //push.transform = projectionView * obj.transform.mat4();
            push.modelMatrix = projectionView * obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();
            push.isSelected = 1;  // 하이라이트에서는 항상 선택됨
            push.enableLighting = enableLighting ? 1 : 0;  // 조명 플래그 전달

            vkCmdPushConstants(
                commandBuffer, pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(SimplePushConstantData), &push);

            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }

    }
}