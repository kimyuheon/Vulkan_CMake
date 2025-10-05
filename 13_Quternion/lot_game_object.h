#pragma once

#include "lot_model.h"

// libs
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

#include <glm/gtc/quaternion.hpp>

namespace lot {
    struct Transformcomponent {
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // 쿼터니언으로 변경
    
        glm::mat4 mat4() const {
            glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 R = glm::mat4_cast(rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
            return T * R * S;
        }

        // 편의 함수들 추가 (오일러 각도로 회전 설정)
        void setRotationEuler(float x, float y, float z) {
            glm::quat qx = glm::angleAxis(x, glm::vec3(1, 0, 0));
            glm::quat qy = glm::angleAxis(y, glm::vec3(0, 1, 0));
            glm::quat qz = glm::angleAxis(z, glm::vec3(0, 0, 1));
            rotation = qy * qx * qz;  // Y-X-Z 순서
        }

        // 오일러 각도 추출 (기존 코드 호환성용)
        glm::vec3 getEulerAngles() const {
            return glm::eulerAngles(rotation);
        }

        // 축 기준 회전 추가
        void rotateAroundAxis(float angle, const glm::vec3& axis) {
            glm::quat rotQuat = glm::angleAxis(angle, axis);
            rotation = rotation * rotQuat;
        }

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

    class LotGameObject {
        public:
            using id_t = unsigned int;

            static LotGameObject createGameObject() {
                static id_t currentId = 0;
                return LotGameObject{currentId++};
            }

            LotGameObject(const LotGameObject &) = delete;
            LotGameObject& operator=(const LotGameObject &) = delete;
            LotGameObject(LotGameObject &&) = default;
            LotGameObject& operator=(LotGameObject &&) = default;

            id_t getId() const { return id; }

            std::shared_ptr<LotModel> model{};
            glm::vec3 color{};
            Transformcomponent transform{};
            bool isSelected{false};

        private:
            LotGameObject(id_t objId) : id{objId} {}

            id_t id;
    };
} // namespace lot