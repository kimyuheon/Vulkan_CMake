#include "lot_buffer.h"

// std
#include <cassert>
#include <cstring>

namespace lot {
    LotBuffer::LotBuffer(LotDevice& device, VkDeviceSize instanceSize, uint32_t instanceCount, 
        VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment) 
        : lotDevice{device}, instanceSize{instanceSize}, instanceCount{instanceCount},
          usageFlags{usageFlags}, memoryPropertyFlags {memoryPropertyFlags} {
        alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
        bufferSize = alignmentSize * instanceCount;
        device.createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer, memory);
    }

    LotBuffer::~LotBuffer() {
        unmap();
        vkDestroyBuffer(lotDevice.device(), buffer, nullptr);
        vkFreeMemory(lotDevice.device(), memory, nullptr);
    }

    VkDeviceSize LotBuffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minoffsetAlignment) {
        if (minoffsetAlignment > 0) {
            return (instanceSize + minoffsetAlignment - 1) & ~(minoffsetAlignment - 1);
        } 
        return instanceSize;
    }

    VkResult LotBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
        assert(buffer && memory && "Called map on buffer before create");
        return vkMapMemory(lotDevice.device(), memory, offset, size, 0, &mapped);
    }

    void LotBuffer::unmap() {
        if (mapped) {
            vkUnmapMemory(lotDevice.device(), memory);
            mapped = nullptr;
        }
    }

    void LotBuffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
        assert(mapped && "Cannot copy to unmapped buffer");

        if(size == VK_WHOLE_SIZE) {
            memcpy(mapped, data, bufferSize);
        } else {
            char* memOffset = (char *)mapped;
            memOffset += offset;
            memcpy(memOffset, data, size);
        }
    }

    VkResult LotBuffer::flush(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(lotDevice.device(), 1, &mappedRange);
    }

    VkDescriptorBufferInfo LotBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
        return VkDescriptorBufferInfo{ buffer, offset, size, };
    }

    VkResult LotBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(lotDevice.device(), 1, &mappedRange);
    }

    void LotBuffer::writeToIndex(void* data, int index) {
        writeToBuffer(data, instanceSize, index * alignmentSize);
    }

    VkResult LotBuffer::flushIndex(int index) {
        return flush(alignmentSize, index * alignmentSize);
    }

    VkDescriptorBufferInfo LotBuffer::descriptorInfoForIndex(int index) {
        return descriptorInfo(alignmentSize, index * alignmentSize);
    }
    
    VkResult LotBuffer::invalidateIndex(int index) {
        return invalidate(alignmentSize, index * alignmentSize);
    }
}