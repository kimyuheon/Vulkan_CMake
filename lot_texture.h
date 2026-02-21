#pragma once

#include "lot_device.h"

#include <string>
#include <memory>

namespace lot {
    class LotTexture {
        public:
            LotTexture(LotDevice& device, const std::string& filpath);
            ~LotTexture();

            LotTexture(const LotTexture&) = delete;
            LotTexture& operator=(const LotTexture&) = delete;
            LotTexture(LotTexture&&) = delete;
            LotTexture& operator=(LotTexture&&) = delete;

            VkImageView getImageView() const { return imageView; }
            VkSampler getSampler() const { return sampler; }

            VkDescriptorImageInfo descriptorInfo() const {
                VkDescriptorImageInfo info{};
                info.sampler = sampler;
                info.imageView = imageView;
                info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                return info;
            }

            static std::unique_ptr<LotTexture> createTextureFromFile(
                LotDevice& device, const std::string filepath);

            static std::unique_ptr<LotTexture> createDefaultTexture(LotDevice& device);

            int getWidth() const { return width; }
            int getHeight() const { return height; }

        private:
            // private 기본 생성자 (createDefaultTexture 전용)
            LotTexture(LotDevice& device) : lotDevice{device} {}
            
            void createTextureImage(const std::string filepath);
            void createTextureImageFromData(const unsigned char* pixels, int w, int h);
            void createTextureimageView();
            void createTextureSampler();
            
            void transitionImageLayout(VkImage image, VkFormat format, 
                                        VkImageLayout oldlayout, VkImageLayout newlayout);
            void copyBufferToImage(VkBuffer buffer, VkImage image, 
                                   uint32_t w, uint32_t h);

            LotDevice& lotDevice;
            
            VkImage textureImage = VK_NULL_HANDLE;
            VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;

            int width = 0;
            int height = 0;
            int channels = 0;
    };
}