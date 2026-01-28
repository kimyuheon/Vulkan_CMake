#pragma once

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace lot {
    class LotCamera {
        public:
            enum class CadViewType {
                Top,      // 평면도 (Top View)
                Front,    // 정면도 (Front View)
                Right,    // 우측면도 (Right Side View)
                Isometric // 등각도 (Isometric View)
            };

            void setOrthographicProjection(
                float left, float right, float top, float bottom, float near, float far);
            void setPerspectiveProjection(float fovy, float aspect, float near, float far);
            
            void setViewDirection(glm::vec3 position, glm::vec3 direction,
                                  glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
            void setViewTarget(glm::vec3 position, glm::vec3 target,
                               glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
            void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
            void setViewFromTransform(glm::vec3 position, glm::quat rotation);
            void setViewFromOrbit(glm::vec3 position, glm::vec3 target, glm::quat additionalRotation);
            void setOrthoSize(float size) { orthographicSize = size; }
            float getOrthoSize() const { return orthographicSize; }

            // CAD 관련 메서드
            void setCadMode(bool enabled) { isCadMode = enabled; }
            bool getCadMode() const { return isCadMode; }
            
            void orbitAroundTarget(float deltaX, float deltaY);  // 쿼터니언 회전
            void zoomToTarget(float delta, bool isOrtho = false, glm::vec3* mouseWorldPos = nullptr);
            void panTarget(float deltaX, float deltaY, bool isOrtho = false, float aspect = 1.0f);
            void setTarget(glm::vec3 newTarget);
            void updateCadView();
            void resetCadRotation(CadViewType viewType = CadViewType::Top);  // 뷰 리셋 기능
            CadViewType getCurrentViewType() const { return currentViewType; }
            glm::vec3 getTarget() const { return targetPosition; }

            const glm::mat4& getProjection() const { return projectionMatrix; }
            const glm::mat4& getView() const { return viewMatrix; }
            glm::mat4 getInverseView() const { return glm::inverse(viewMatrix); }
            glm::vec3 getPosition() const {
                glm::mat4 inverse = glm::inverse(viewMatrix);
                return glm::vec3(inverse[3]);
            }
        private:
            glm::mat4 projectionMatrix{1.f};
            glm::mat4 viewMatrix{1.f};

            // CAD 모드 변수
            bool isCadMode{false};
            glm::vec3 targetPosition{0.f, 0.f, 0.f};
            float orbitDistance{10.f};
            glm::quat orbitRotation{1.0f, 0.0f, 0.0f, 0.0f};  // 누적 회전 (쿼터니언) #include <glm/gtc/quaternion.hpp>
            float orthographicSize{5.f};
            CadViewType currentViewType{CadViewType::Front};  // 현재 CAD 뷰 타입
    };
} // namespace lot