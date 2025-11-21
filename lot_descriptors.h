#pragma once

#include "lot_device.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace lot {
    class LotDescriptorSetLayout {
        public:
            class Builder {
                public:
                    Builder(LotDevice& lotDevice) : lotDevice{lotDevice} {}

                    Builder& addBinding(
                        uint32_t binding,
                        VkDescriptorType descriptorType,
                        VkShaderStageFlags stageFlags,
                        uint32_t count = 1);
                    std::unique_ptr<LotDescriptorSetLayout> build() const;
                private:
                    LotDevice& lotDevice;
                    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
            };

        LotDescriptorSetLayout(LotDevice& lotDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~LotDescriptorSetLayout();
        LotDescriptorSetLayout(const LotDescriptorSetLayout&) = delete;
        LotDescriptorSetLayout& operator=(const LotDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

        private:
            LotDevice& lotDevice;
            VkDescriptorSetLayout descriptorSetLayout;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

            friend class LotDescriptorWriter;
    };

    class LotDescriptorPool {
        public:
            class Builder {
                public:
                    Builder(LotDevice& lotDevice) : lotDevice{lotDevice} {}

                    Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
                    Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
                    Builder& setMaxSets(uint32_t count);
                    std::unique_ptr<LotDescriptorPool> build() const;

                private:
                    LotDevice& lotDevice;
                    std::vector<VkDescriptorPoolSize> poolSizes{};
                    uint32_t maxSets = 1000;
                    VkDescriptorPoolCreateFlags poolFlags = 0;
            };
            LotDescriptorPool(LotDevice& lotDevice, uint32_t maxSets, 
                              VkDescriptorPoolCreateFlags poolFlags, 
                              const std::vector<VkDescriptorPoolSize>& poolSizes);
            ~LotDescriptorPool();
            LotDescriptorPool(const LotDescriptorPool&) = delete;
            LotDescriptorPool& operator=(const LotDescriptorPool&) = delete;

            bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, 
                                     VkDescriptorSet& descriptor) const;
            void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
            void resetPool();
        private:
            LotDevice& lotDevice;
            VkDescriptorPool descriptorPool;

            friend class LotDescriptorWriter;
    };

    class LotDescriptorWriter {
        public:
            LotDescriptorWriter(LotDescriptorSetLayout& setLayout, LotDescriptorPool& pool);

            LotDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
            LotDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

            bool build(VkDescriptorSet& set);
            void overwrite(VkDescriptorSet& set);

        private:
            LotDescriptorSetLayout& setLayout;
            LotDescriptorPool& pool;
            std::vector<VkWriteDescriptorSet> writes;
    };
}