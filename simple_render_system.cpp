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
        // JFA로 대체
        //createPipeline(renderPass, OutlineType::Higtlight);
        //createPipeline(renderPass, OutlineType::Outline);
        //createHighlightPipeline(renderPass);
        //createOutlinePipeline(renderPass);
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

    void SimpleRenderSystem::createPipeline(VkRenderPass renderPass, OutlineType type) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LotPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        if (type == OutlineType::Higtlight) {
            pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
            pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        } else if (type == OutlineType::Outline) {
            pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        }

        if (type != OutlineType::Default) {
            pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
            pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
            pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        }

        // 스텐실: 선택된 오브젝트만 stencil=1 기록 (dynamic reference 사용)
        VkStencilOpState stencilWrite{};
        stencilWrite.failOp = VK_STENCIL_OP_KEEP;
        //stencilWrite.passOp = VK_STENCIL_OP_REPLACE;
        stencilWrite.depthFailOp = VK_STENCIL_OP_KEEP;
        //stencilWrite.compareOp = VK_COMPARE_OP_ALWAYS;
        stencilWrite.compareMask = 0xFF;
        //stencilWrite.writeMask = 0xFF;
        //stencilWrite.reference = 0;
        
        if (type == OutlineType::Default) {
            stencilWrite.passOp = VK_STENCIL_OP_REPLACE;
            stencilWrite.compareOp = VK_COMPARE_OP_ALWAYS;
            stencilWrite.writeMask = 0xFF;
            stencilWrite.reference = 0;
        } else if (type == OutlineType::Higtlight || type == OutlineType::Outline) {
            stencilWrite.passOp = VK_STENCIL_OP_KEEP;
            stencilWrite.compareOp = VK_COMPARE_OP_NOT_EQUAL;
            stencilWrite.writeMask = 0x00;
            stencilWrite.reference = 1;
        } 
        
        pipelineConfig.depthStencilInfo.stencilTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.front = stencilWrite;
        pipelineConfig.depthStencilInfo.back = stencilWrite;        

        // dynamic stencil reference 추가
        if (type == OutlineType::Default) {
            pipelineConfig.dynamicStateEnables.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
            pipelineConfig.dynamicStateInfo.pDynamicStates = pipelineConfig.dynamicStateEnables.data();
            pipelineConfig.dynamicStateInfo.dynamicStateCount =
                static_cast<uint32_t>(pipelineConfig.dynamicStateEnables.size());
        }

        if (type == OutlineType::Default) {
            lotPipeline = std::make_unique<LotPipeline>(
                lotDevice,
                "shaders/simple_shader.vert.spv",
                "shaders/simple_shader.frag.spv" ,
                pipelineConfig);
        } 
        // JFA로 대체
        /* else if (type == OutlineType::Higtlight) {
            highlightPipeline = std::make_unique<LotPipeline>(
                lotDevice,
                "shaders/simple_shader.vert.spv",
                "shaders/simple_shader.frag.spv" ,
                pipelineConfig);
        } else if (type == OutlineType::Outline) {
            outlinePipeline = std::make_unique<LotPipeline>(
                lotDevice,
                "shaders/simple_shader.vert.spv",
                "shaders/simple_shader.frag.spv" ,
                pipelineConfig);
        } */
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

    /* void SimpleRenderSystem::createHighlightPipeline(VkRenderPass renderPass) {
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
    } */

    /* void SimpleRenderSystem::createOutlinePipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create outline pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LotPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        // 컬링 없음: 모든 면 렌더링 (스크린 공간 법선 + 중심 폴백으로 아웃라인 처리)
        pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

        pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        // 스텐실: 오브젝트가 그려진 영역(stencil=1)은 제외, 외곽만 그리기
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

        outlinePipeline = std::make_unique<LotPipeline>(
            lotDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig);
    } */

    void SimpleRenderSystem::renderHighlights(FrameInfo &frameInfo) {
        // ── 1. 3D 오브젝트 아웃라인 (프론트페이스 컬링 → 백페이스 실루엣) ──
        outlinePipeline->bind(frameInfo.commandBuffer);
        renderOutlineLoop(frameInfo, 2);
        /* vkCmdBindDescriptorSets(
            frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

        for (auto& kv : frameInfo.Objects) {
            auto& obj = kv.second;
            if (!obj.isSelected || !obj.model || obj.model->isFlat()) continue;

            if (!obj.textureDescriptorSets.empty()) {
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout, 1, 1, &obj.textureDescriptorSets[frameInfo.frameIndex],
                    0, nullptr);
            }

            SimplePushConstantData push{};
            push.modelMatrix  = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();
            push.isSelected   = 2;  // 백페이스 아웃라인

            vkCmdPushConstants(
                frameInfo.commandBuffer, pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(SimplePushConstantData), &push);

            obj.model->bind(frameInfo.commandBuffer);
            obj.model->draw(frameInfo.commandBuffer);
        } */

        // ── 2. 평면 오브젝트 아웃라인 (컬링 없음 → 방사형 확장) ──
        highlightPipeline->bind(frameInfo.commandBuffer);
        renderOutlineLoop(frameInfo, 3);
        /* vkCmdBindDescriptorSets(
            frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

        int highlightCount = 0;
        for (auto& kv : frameInfo.Objects) {
            auto& obj = kv.second;
            if (!obj.isSelected || !obj.model || !obj.model->isFlat()) continue;
            highlightCount++;

            if (!obj.textureDescriptorSets.empty()) {
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout, 1, 1, &obj.textureDescriptorSets[frameInfo.frameIndex],
                    0, nullptr);
            }

            SimplePushConstantData push{};
            push.modelMatrix  = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();
            push.isSelected   = 3;  // 방사형 확장

            vkCmdPushConstants(
                frameInfo.commandBuffer, pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(SimplePushConstantData), &push);

            obj.model->bind(frameInfo.commandBuffer);
            obj.model->draw(frameInfo.commandBuffer);
        } */

        /* if (highlightCount > 0) {
            static int logCount = 0;
            if (logCount++ % 300 == 0) {
                std::cout << "[Highlight] Rendering " << highlightCount << " selected object(s)" << std::endl;
            }
        } */
    }

    void SimpleRenderSystem::renderOutlineLoop(FrameInfo& frameInfo, int isSelected) {
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

        int highlightCount = 0;
        bool wantFlat = (isSelected == 3);
        for (auto& kv : frameInfo.Objects) {
            auto& obj = kv.second;
            
            if (!obj.isSelected || !obj.model || obj.model->isFlat() != wantFlat) continue;
            
            highlightCount++;

            if (!obj.textureDescriptorSets.empty()) {
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout, 1, 1, &obj.textureDescriptorSets[frameInfo.frameIndex],
                    0, nullptr);
            }

            SimplePushConstantData push{};
            push.modelMatrix  = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();
            push.isSelected   = isSelected;  // 2: 백페이스 아웃라인 / 3: 방사형 확장

            vkCmdPushConstants(
                frameInfo.commandBuffer, pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(SimplePushConstantData), &push);

            obj.model->bind(frameInfo.commandBuffer);
            obj.model->draw(frameInfo.commandBuffer);

            /* if (highlightCount > 0) {
            static int logCount = 0;
            if (logCount++ % 300 == 0) {
                std::cout << "[Highlight] Rendering " << highlightCount << " selected object(s)" << std::endl;
            }
        } */
        }
    }
}