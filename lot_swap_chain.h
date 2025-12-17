#pragma once

#include "lot_device.h"

// Vulkan header
#include "vulkan/vulkan.h"

#include <memory>
#include <string>
#include <vector>

namespace lot {
    class LotSwapChain {
        public:
            static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

            LotSwapChain(LotDevice &deviceRef, VkExtent2D extent);
            LotSwapChain(LotDevice &deviceRef, VkExtent2D extent, std::shared_ptr<LotSwapChain> previous);
            ~LotSwapChain();

            LotSwapChain(const LotSwapChain &) = delete;
            LotSwapChain& operator=(const LotSwapChain &) = delete;

            VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
            VkRenderPass getRenderPass() { return renderPass; }
            VkImageView getImageView(int index) { return swapChainImageViews[index]; }
            size_t imageCount() { return swapChainImages.size(); }
            VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
            VkExtent2D getSwapChainExtent() { return swapChainExtent; }
            uint32_t width() { return swapChainExtent.width; }
            uint32_t height() { return swapChainExtent.height; }

            float extentAspectRatio() {
                return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
            }

            VkFormat findDepthFormat();

            VkResult acquireNextImage(uint32_t *imageIndex);
            VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

            bool compareSwapFormats(const LotSwapChain& swapChain) const {
                return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
                       swapChain.swapChainImageFormat == swapChainImageFormat;
            }

        private:
            void init();
            void createSwapChain();
            void createImageViews();
            void createDepthResource();
            void createRenderPass();
            void createFramebuffers();
            void createSyncObjects();

            // Helper functions
            VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
            VkPresentModeKHR chooseSwapPresentmode(const std::vector<VkPresentModeKHR> &availablePresentModes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

            VkFormat swapChainImageFormat;
            VkFormat swapChainDepthFormat;
            VkExtent2D swapChainExtent;

            std::vector<VkFramebuffer> swapChainFramebuffers;
            VkRenderPass renderPass;

            std::vector<VkImage> depthImages;
            std::vector<VkDeviceMemory> depthImageMemorys;
            std::vector<VkImageView> depthImageViews;
            std::vector<VkImage> swapChainImages;
            std::vector<VkImageView> swapChainImageViews;

            LotDevice &device;
            VkExtent2D windowExtent;

            VkSwapchainKHR swapChain;
            std::shared_ptr<LotSwapChain> oldSwapChain;

            std::vector<VkSemaphore> imageAvailableSemaphores;
            std::vector<VkSemaphore> renderFinishedSemaphores;
            std::vector<VkFence> inFlightFences;
            std::vector<VkFence> imagesInFlight;
            std::vector<uint32_t> frameSemaphoreIndices;  // 각 프레임이 사용한 세마포어 인덱스
            size_t currentFrame = 0;
            uint32_t nextSemaphoreIndex = 0;  // 다음에 사용할 세마포어 인덱스
    };
} // namespace lot
