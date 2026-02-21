#pragma once

#include "lot_model.h"
#include "lot_texture.h"

// libs
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <unordered_map>

namespace lot {
    class LotTexture;
    struct Transformcomponent {
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // 쿼터니언으로 변경
    
        glm::mat4 mat4() const;

        glm::mat4 normalMatrix() const;

        // 편의 함수들 추가 (오일러 각도로 회전 설정)
        void setRotationEuler(float x, float y, float z);

        // 오일러 각도 추출 (기존 코드 호환성용)
        glm::vec3 getEulerAngles() const {
            return glm::eulerAngles(rotation);
        }

        // 축 기준 회전 추가
        void rotateAroundAxis(float angle, const glm::vec3& axis);

        // === 패치에서 제안된 쿼터니언 헬퍼 함수들 ===

        // 로컬 좌표계 기준 회전 (기존 회전에 후곱)
        void rotateLocal(float angle, const glm::vec3& axis) {
            rotation = glm::normalize(rotation * glm::angleAxis(angle, glm::normalize(axis)));
        }

        // 월드 좌표계 기준 회전 (기존 회전에 전곱)
        void rotateWorld(float angle, const glm::vec3& axis) {
            rotation = glm::normalize(glm::angleAxis(angle, glm::normalize(axis)) * rotation);
        }

        // 현재 회전 상태에서의 방향 벡터들
        glm::vec3 right() const   { return glm::mat3_cast(rotation) * glm::vec3(1,0,0); }
        glm::vec3 up() const      { return glm::mat3_cast(rotation) * glm::vec3(0,1,0); }
        glm::vec3 forward() const { return glm::mat3_cast(rotation) * glm::vec3(0,0,-1); }
    };

    struct PointLightComponent {
        float lightIntensity = 1.0f;
    };

    class LotGameObject {
        public:
            using id_t = unsigned int;
            using Map = std::unordered_map<id_t, LotGameObject>;

            static LotGameObject createGameObject() {
                static id_t currentId = 0;
                return LotGameObject{currentId++};
            }

            static LotGameObject makePointLight(
                float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));

            LotGameObject(const LotGameObject &) = delete;
            LotGameObject& operator=(const LotGameObject &) = delete;
            LotGameObject(LotGameObject &&) = default;
            LotGameObject& operator=(LotGameObject &&) = default;

            id_t getId() const { return id; }

            std::shared_ptr<LotModel> model{};
            std::unique_ptr<PointLightComponent> pointLight = nullptr;

            glm::vec3 color{};
            Transformcomponent transform{};
            bool isSelected{false};

            LotTexture* texture = nullptr;
            std::string textureName = "";
            float textureScale = 1.0f;
            std::vector<VkDescriptorSet> textureDescriptorSets;

        private:
            LotGameObject(id_t objId) : id{objId} {}

            id_t id;
    };
} // namespace lot