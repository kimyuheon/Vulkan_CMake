#pragma once

#include "lot_device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <cstring>

namespace lot {
    class LotModel {
        public:
            struct  Vertex
            {
                glm::vec3 position{};
                glm::vec3 color{};
                glm::vec3 normal{};
                glm::vec2 uv{};

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

                bool operator==(const Vertex& other) const {
                    return position == other.position && 
                           color == other.color && 
                           normal == other.normal &&
                           uv == other.uv;
                }
            };

            struct BoundingBox {
                glm::vec3 min{std::numeric_limits<float>::max()};
                glm::vec3 max{std::numeric_limits<float>::lowest()};
            };

            struct Builder {
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices{};
                void loadModel(const std::string& filepath);
            };
            
            LotModel(LotDevice &device, const LotModel::Builder &builder);
            ~LotModel();

            LotModel(const LotModel &) = delete;
            LotModel &operator=(const LotModel &) = delete;

            static std::unique_ptr<LotModel> createModelFromFile(
                LotDevice &device, 
                const std::string &filepath);

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

            // 레이캐스팅을 위한 메시 데이터 접근
            const std::vector<Vertex>& getVertices() const { return vertices; }
            const std::vector<uint32_t>& getIndices() const { return indices; }
            bool hasIndices() const { return hasIndexBuffer; }

            // 로컬 공간에서의 바운딩박스 (Transform 적용 전)
            const BoundingBox& getLocalBoundingBox() const { return localBoundingBox; }

            // 평면 오브젝트 여부 (바운딩박스 기반 자동 감지)
            bool isFlat() const {
                glm::vec3 extent = localBoundingBox.max - localBoundingBox.min;
                float minDim = glm::min(extent.x, glm::min(extent.y, extent.z));
                float maxDim = glm::max(extent.x, glm::max(extent.y, extent.z));
                return maxDim > 0.001f && (minDim / maxDim) < 0.05f;
            }

        private:
            void createVertexBuffers(const std::vector<Vertex> &vertices);
            void createIndexBuffers(const std::vector<uint32_t> &indices);
            void calculateBoundingBox();

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

            // 로컬 공간 바운딩박스 (모델 로드 시 계산)
            BoundingBox localBoundingBox;
    };
}