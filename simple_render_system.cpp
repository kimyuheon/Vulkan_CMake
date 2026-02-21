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
        alignas(4) int isSelected{0};           // 선택 상태 플래그 추가
        //alignas(4) int enableLighting{1};     // 조명 활성화 플래그 추가
        alignas(4) int hasTexture{0};           // 텍스터 적용 여부
        alignas(4) float textureScale{1.f};     // 텍스터 크기
    };
    

    SimpleRenderSystem::SimpleRenderSystem(LotDevice &device, VkRenderPass renderPass, 
                                           VkDescriptorSetLayout globalSetLayout,
                                           VkDescriptorSetLayout textureSetLayout)
    : lotDevice{device}  {
        createPipelineLayout(globalSetLayout, textureSetLayout);
        createPipeline(renderPass);
        createHighlightPipeline(renderPass);
    }

    SimpleRenderSystem::~SimpleRenderSystem() {
        vkDeviceWaitIdle(lotDevice.device());
        vkDestroyPipelineLayout(lotDevice.device(), pipelineLayout, nullptr);
    }

    void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout,
                                                  VkDescriptorSetLayout textureSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout, textureSetLayout };

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

    void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LotPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        // 스텐실: 선택된 오브젝트만 stencil=1 기록 (dynamic reference 사용)
        VkStencilOpState stencilWrite{};
        stencilWrite.failOp = VK_STENCIL_OP_KEEP;
        stencilWrite.passOp = VK_STENCIL_OP_REPLACE;
        stencilWrite.depthFailOp = VK_STENCIL_OP_KEEP;
        stencilWrite.compareOp = VK_COMPARE_OP_ALWAYS;
        stencilWrite.compareMask = 0xFF;
        stencilWrite.writeMask = 0xFF;
        stencilWrite.reference = 0;
        pipelineConfig.depthStencilInfo.stencilTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.front = stencilWrite;
        pipelineConfig.depthStencilInfo.back = stencilWrite;

        // dynamic stencil reference 추가
        pipelineConfig.dynamicStateEnables.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
        pipelineConfig.dynamicStateInfo.pDynamicStates = pipelineConfig.dynamicStateEnables.data();
        pipelineConfig.dynamicStateInfo.dynamicStateCount =
            static_cast<uint32_t>(pipelineConfig.dynamicStateEnables.size());

        lotPipeline = std::make_unique<LotPipeline>(
            lotDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv" ,
            pipelineConfig);
    }

    void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo) {
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
            //obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.0001f, glm::two_pi<float>());
            //obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.00005f, glm::two_pi<float>());
            auto& obj = kv.second;
            if (obj.model == nullptr) continue;

            // 선택된 오브젝트: stencil=1, 비선택: stencil=0
            uint32_t stencilRef = obj.isSelected ? 1 : 0;
            vkCmdSetStencilReference(frameInfo.commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, stencilRef);

            bool objHasTexture = (obj.texture != nullptr && !obj.textureDescriptorSets.empty());

            if (!obj.textureDescriptorSets.empty()) {
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    1, 1, &obj.textureDescriptorSets[frameInfo.frameIndex],
                    0, nullptr);
            }

            SimplePushConstantData push{};
            push.color = obj.color;
            //push.transform = projectionView * obj.transform.mat4();
            push.modelMatrix = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();
            push.isSelected = obj.isSelected ? 1 : 0;  // 선택 상태 전달
            push.hasTexture = objHasTexture? 1 : 0;
            push.textureScale = obj.textureScale;

            vkCmdPushConstants(
                frameInfo.commandBuffer, pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(SimplePushConstantData), &push);

            obj.model->bind(frameInfo.commandBuffer);
            obj.model->draw(frameInfo.commandBuffer);
        }
    }

    void SimpleRenderSystem::createHighlightPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create highlight pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LotPipeline::defaultPipelineConfigInfo(pipelineConfig);

        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;

        // 깊이: LESS_OR_EQUAL (바닥과 같은 깊이에서도 아웃라인 표시)
        pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        // 스텐실: stencil != 1 인 곳에만 그리기 (선택된 오브젝트 영역 제외)
        VkStencilOpState stencilTest{};
        stencilTest.failOp = VK_STENCIL_OP_KEEP;
        stencilTest.passOp = VK_STENCIL_OP_KEEP;
        stencilTest.depthFailOp = VK_STENCIL_OP_KEEP;
        stencilTest.compareOp = VK_COMPARE_OP_NOT_EQUAL;
        stencilTest.compareMask = 0xFF;
        stencilTest.writeMask = 0x00;
        stencilTest.reference = 1;
        pipelineConfig.depthStencilInfo.stencilTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.front = stencilTest;
        pipelineConfig.depthStencilInfo.back = stencilTest;

        highlightPipeline = std::make_unique<LotPipeline>(
            lotDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig);
    }

    void SimpleRenderSystem::renderHighlights(FrameInfo &frameInfo) {
        highlightPipeline->bind(frameInfo.commandBuffer);

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

        int highlightCount = 0;
        for (auto& kv : frameInfo.Objects) {
            auto& obj = kv.second;
            if (!obj.isSelected) continue;
            highlightCount++;

            if (!obj.textureDescriptorSets.empty()) {
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    1, 1, &obj.textureDescriptorSets[frameInfo.frameIndex],
                    0, nullptr);
            }

            SimplePushConstantData push{};
            //push.color = glm::vec3(1.0f, 0.5f, 0.0f);  // 주황색 하이라이트
            //push.transform = projectionView * obj.transform.mat4();
            push.modelMatrix = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();
            // 평면 오브젝트는 방사형 확장(3), 3D 오브젝트는 노멀 확장(2)
            push.isSelected = (obj.model && obj.model->isFlat()) ? 3 : 2;

            vkCmdPushConstants(
                frameInfo.commandBuffer, pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(SimplePushConstantData), &push);

            obj.model->bind(frameInfo.commandBuffer);
            obj.model->draw(frameInfo.commandBuffer);
        }

        /* if (highlightCount > 0) {
            static int logCount = 0;
            if (logCount++ % 300 == 0) {
                std::cout << "[Highlight] Rendering " << highlightCount << " selected object(s)" << std::endl;
            }
        } */
    }
}