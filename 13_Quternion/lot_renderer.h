#pragma once

#include "lot_device.h"
#include "lot_swap_chain.h"
#include "lot_window.h"

#include <cassert>
#include <memory>
#include <vector>

namespace lot {
    class LotRenderer {
        public:
            LotRenderer(LotWindow &window, LotDevice &device);
            ~LotRenderer();

            LotRenderer(const LotRenderer &) = delete;
            LotRenderer &operator=(const LotRenderer&) = delete;

            VkRenderPass getSwapChainRenderPass() const { return lotSwapChain->getRenderPass(); }
            float getAspectRatio() const { return lotSwapChain->extentAspectRatio(); }
            bool isFrameInProgress() const { return isFrameStarted; }

            VkCommandBuffer getCurrentCommandBuffer() const {
                assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
                return commandBuffers[currentFrameIndex];
            }

            int getFrameIndex() const {
                assert(isFrameStarted && "Cannot get frame index when frame not in progress");
                return currentFrameIndex;
            }

            VkCommandBuffer beginFrame();
            void endFrame();
            void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
            void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

        private:
            void createCommandBuffers();
            void freeCommandBuffers();
            void recreateSwapChain();

            LotWindow& lotWindow;
            LotDevice& lotDevice;
            std::unique_ptr<LotSwapChain> lotSwapChain;
            std::vector<VkCommandBuffer> commandBuffers;

            uint32_t currentImageIndex;
            int currentFrameIndex{0};
            bool isFrameStarted{false};
    };
} // namespace lot