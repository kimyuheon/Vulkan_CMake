#pragma once

#include "lot_device.h"
#include "lot_pipeline.h"
#include "lot_frame_info.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace lot {
    enum class SelectionType : int {
        SelectionMask, Outline
    };
    class JFARenderSystem {
    public:
        // mainRenderPass : lotRenderer.getSwapChainRenderPass()
        // globalSetLayout: set 0 descriptor layout (UBO)
        // frameCount     : lotSwapChain->imageCount()
        JFARenderSystem(LotDevice& device,
                        VkRenderPass mainRenderPass,
                        VkExtent2D extent,
                        int frameCount,
                        VkDescriptorSetLayout globalSetLayout);
        ~JFARenderSystem();

        JFARenderSystem(const JFARenderSystem&) = delete;
        JFARenderSystem& operator=(const JFARenderSystem&) = delete;

        // Call BEFORE beginSwapChainRenderPass: renders selected objects to mask texture
        void renderSelectionMask(VkCommandBuffer cmd, int frameIndex,
                                 FrameInfo& frameInfo);

        // Call AFTER renderSelectionMask, BEFORE beginSwapChainRenderPass
        void runJFA(VkCommandBuffer cmd, int frameIndex);

        // Call INSIDE swapchain renderpass (replaces renderHighlights)
        void renderOutline(VkCommandBuffer cmd, int frameIndex);

    private:
        // ---------- per-frame GPU resources ----------
        struct FrameResources {
            // Selection mask (COLOR_ATTACHMENT + SAMPLED)
            VkImage        maskImage{VK_NULL_HANDLE};
            VkDeviceMemory maskMemory{VK_NULL_HANDLE};
            VkImageView    maskView{VK_NULL_HANDLE};
            VkFramebuffer  maskFramebuffer{VK_NULL_HANDLE};

            // Depth for selection mask renderpass
            VkImage        depthImage{VK_NULL_HANDLE};
            VkDeviceMemory depthMemory{VK_NULL_HANDLE};
            VkImageView    depthView{VK_NULL_HANDLE};

            // JFA ping-pong (STORAGE + SAMPLED), [0] and [1]
            VkImage        jfaImage[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
            VkDeviceMemory jfaMemory[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
            VkImageView    jfaView[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};

            // Compute descriptor sets
            VkDescriptorSet initSet{VK_NULL_HANDLE};   // mask  → jfa[0]
            VkDescriptorSet atobSet{VK_NULL_HANDLE};   // jfa[0]→ jfa[1]
            VkDescriptorSet btoaSet{VK_NULL_HANDLE};   // jfa[1]→ jfa[0]

            // Outline descriptor set (reads final JFA result)
            VkDescriptorSet outlineSet{VK_NULL_HANDLE};
        };

        // ---------- setup helpers ----------
        void createImages(int frameCount);
        void createSelectionMaskRenderPass();
        void createSelectionMaskFramebuffers(int frameCount);
        void createSampler();
        void createComputeDescriptorLayout();
        void createOutlineDescriptorLayout();
        void createDescriptorPool(int frameCount);
        void allocateDescriptorSets(int frameCount);
        void createComputePipeline();
        void createSelectionMaskPipeline(VkDescriptorSetLayout globalSetLayout);
        void createOutlinePipeline(VkRenderPass mainRenderPass);
        void destroyImages();

        // ---------- barrier helper ----------
        void imageBarrier(VkCommandBuffer cmd, VkImage image,
                          VkImageAspectFlags aspect,
                          VkImageLayout oldLayout, VkImageLayout newLayout,
                          VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                          VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

        // ---------- members ----------
        LotDevice&  lotDevice;
        VkExtent2D  extent;
        int         numJfaSteps;   // ceil(log2(max(w,h)))
        int         finalJfaIdx;   // jfa result buffer index after numJfaSteps

        VkRenderPass selectionMaskRenderPass{VK_NULL_HANDLE};
        VkSampler    sampler{VK_NULL_HANDLE};

        // Compute pipeline
        VkDescriptorSetLayout computeDescLayout{VK_NULL_HANDLE};
        VkDescriptorPool      computePool{VK_NULL_HANDLE};
        VkPipelineLayout      jfaComputeLayout{VK_NULL_HANDLE};
        VkPipeline            jfaComputePipeline{VK_NULL_HANDLE};

        // Selection mask graphics pipeline
        VkPipelineLayout               maskPipelineLayout{VK_NULL_HANDLE};
        std::unique_ptr<LotPipeline>   maskPipeline;

        // Outline (fullscreen quad) pipeline
        VkDescriptorSetLayout          outlineDescLayout{VK_NULL_HANDLE};
        VkPipelineLayout               outlinePipelineLayout{VK_NULL_HANDLE};
        std::unique_ptr<LotPipeline>   outlinePipeline;

        std::vector<FrameResources> frames;
    };

} // namespace lot
