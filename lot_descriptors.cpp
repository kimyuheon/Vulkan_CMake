#include "lot_descriptors.h"

// std
#include <cassert>
#include <stdexcept>

namespace lot {
    // LotDescriptorSetLayout::Builder
    LotDescriptorSetLayout::Builder& LotDescriptorSetLayout::Builder::addBinding(
        uint32_t binding, VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags, uint32_t count) {
        assert(bindings.count(binding) == 0 && "Binding already in use");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<LotDescriptorSetLayout> LotDescriptorSetLayout::Builder::build() const {
        return std::make_unique<LotDescriptorSetLayout>(lotDevice, bindings);
    }

    // LotDescriptorSetLayout
    LotDescriptorSetLayout::LotDescriptorSetLayout(LotDevice& lotDevice, 
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings) 
    : lotDevice{lotDevice}, bindings{bindings} {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv : bindings) {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(lotDevice.device(), &descriptorSetLayoutInfo, 
                                        nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout");
        }
    }
    LotDescriptorSetLayout::~LotDescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(lotDevice.device(), descriptorSetLayout, nullptr);
    }

    // DescriptorPool::Builder
    LotDescriptorPool::Builder& LotDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, 
                                                                        uint32_t count) {
        poolSizes.push_back({descriptorType, count});
        return *this;
    }

    LotDescriptorPool::Builder& LotDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }

    LotDescriptorPool::Builder& LotDescriptorPool::Builder::setMaxSets(uint32_t count) {
        maxSets = count;
        return *this;
    }
    
    std::unique_ptr<LotDescriptorPool> LotDescriptorPool::Builder::build() const {
        return std::make_unique<LotDescriptorPool>(lotDevice, maxSets, poolFlags, poolSizes);
    }

    // DescriptorPool
    LotDescriptorPool::LotDescriptorPool(LotDevice& lotDevice, uint32_t maxSets, 
                                         VkDescriptorPoolCreateFlags poolFlags, 
                                         const std::vector<VkDescriptorPoolSize>& poolSizes) 
                                         : lotDevice{lotDevice} {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(lotDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }
    
    LotDescriptorPool::~LotDescriptorPool() {
        vkDestroyDescriptorPool(lotDevice.device(), descriptorPool, nullptr);
    }

    bool LotDescriptorPool::allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, 
                                                VkDescriptorSet& descriptor) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        if (vkAllocateDescriptorSets(lotDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
            return false;
        }

        return true;
    }

    void LotDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
        vkFreeDescriptorSets(lotDevice.device(), descriptorPool, 
                             static_cast<uint32_t>(descriptors.size()), descriptors.data());
    }

    void LotDescriptorPool::resetPool() {
        vkResetDescriptorPool(lotDevice.device(), descriptorPool, 0);
    }

    // LotDescriptorWriter
    LotDescriptorWriter::LotDescriptorWriter(LotDescriptorSetLayout& setLayout, LotDescriptorPool& pool) 
    : setLayout{setLayout}, pool{pool} {

    }

    LotDescriptorWriter& LotDescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specifed binding");

        auto &bindingDescription = setLayout.bindings[binding];

        assert(bindingDescription.descriptorCount == 1 && 
              "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        
        return *this;
    }
    LotDescriptorWriter& LotDescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specifed binding");

        auto &bindingDescription = setLayout.bindings[binding];

        assert(bindingDescription.descriptorCount == 1 && 
              "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        
        return *this;
    }

    bool LotDescriptorWriter::build(VkDescriptorSet& set) {
        bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
        if (!success) {
            return false;
        }
        overwrite(set);
        return true;
    }

    void LotDescriptorWriter::overwrite(VkDescriptorSet& set) {
        for (auto &write : writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(pool.lotDevice.device(), writes.size(), writes.data(), 0, nullptr);
    }
}