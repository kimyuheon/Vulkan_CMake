#pragma once

#include "lot_device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <cstring>

namespace lot {
    class LotModel {
        public:
            struct  Vertex
            {
                glm::vec3 position{};
                glm::vec3 color{};

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            };

            struct Builder {
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices{};
            };
            
            LotModel(LotDevice &device, const LotModel::Builder &builder);
            ~LotModel();

            LotModel(const LotModel &) = delete;
            LotModel &operator=(const LotModel &) = delete;

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

            // 레이캐스팅을 위한 메시 데이터 접근
            const std::vector<Vertex>& getVertices() const { return vertices; }
            const std::vector<uint32_t>& getIndices() const { return indices; }
            bool hasIndices() const { return hasIndexBuffer; }

        private:
            void createVertexBuffers(const std::vector<Vertex> &vertices);
            void createIndexBuffers(const std::vector<uint32_t> &indices);

            LotDevice& lotDevice;

            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            uint32_t vertexCount;

            bool hasIndexBuffer = false;
            VkBuffer indexBuffer;
            VkDeviceMemory indexBufferMemory;
            uint32_t indexCount;

            // CPU에서 접근 가능한 메시 데이터 (레이캐스팅용)
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
    };
}