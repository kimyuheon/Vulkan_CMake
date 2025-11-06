#include "lot_camera.h"

// GLM 실험적 확장 기능 활성화
#define GLM_ENABLE_EXPERIMENTAL

// std
#include <cassert>
#include <limits>

// GLM 쿼터니언 헤더
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace lot {
    void LotCamera::setOrthographicProjection(float left, float right, float top, float bottom, 
                                              float near, float far) {
        projectionMatrix = glm::mat4{1.0f};
        projectionMatrix[0][0] = 2.f / (right - left);
        projectionMatrix[1][1] = 2.f / (bottom - top);
        projectionMatrix[2][2] = 1.f / (far - near);
        projectionMatrix[3][0] = -(right + left) / (right - left);
        projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
        projectionMatrix[3][2] = -near / (far - near);
    }

    void LotCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
        assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
        const float tanHalfFovy = tan(fovy / 2.f);
        projectionMatrix = glm::mat4{0.0f};
        projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
        projectionMatrix[1][1] = 1.f / (tanHalfFovy);
        projectionMatrix[2][2] = far / (far - near);
        projectionMatrix[2][3] = 1.f;
        projectionMatrix[3][2] = -(far * near) / (far - near);
    }

    void LotCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
        const glm::vec3 w{glm::normalize(direction)};
        // up 벡터와 forward가 평행한지 체크
        float dot = abs(glm::dot(w, glm::normalize(up)));
        if (dot > 0.99f) {
            up = glm::vec3(1, 0, 0); // 대체 up 벡터
        }
        const glm::vec3 u{glm::normalize(glm::cross(w, up))};
        const glm::vec3 v{glm::cross(w, u)};

        viewMatrix = glm::mat4{1.f};
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);
    }

    void LotCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
        setViewDirection(position, target - position, up);
    }

    void LotCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);
        const glm::vec3 u{(c1 * c3 + s1 * s2 * s3),
                          (c2 * s3),
                          (c1 * s2 * s3 - c3 * s1)};
        const glm::vec3 v{(c3 * s1 * s2 - c1 * s3),
                          (c2 * c3),
                          (c1 * c3 * s2 + s1 * s3)};
        const glm::vec3 w{(c2 * s1),
                          (-s2),
                          (c1 * c2)};
        viewMatrix = glm::mat4{1.f};
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);
    }

    void LotCamera::setViewFromTransform(glm::vec3 position, glm::quat rotation) {
        // 카메라 변환 매트릭스 생성 (T * R)
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotate = glm::mat4_cast(rotation);
        glm::mat4 cameraMatrix = translate * rotate;

        // 뷰 매트릭스 = 카메라 변환의 역행렬
        viewMatrix = glm::inverse(cameraMatrix);
    }

    void LotCamera::setViewFromOrbit(glm::vec3 position, glm::vec3 target, glm::quat additionalRotation) {
        // 1. 기본 방향: 카메라에서 타겟을 향하는 방향
        glm::vec3 baseDirection = glm::normalize(target - position);

        // 2. 추가 회전 적용
        glm::vec3 rotatedDirection = additionalRotation * baseDirection;

        // 3. Up 벡터도 회전 적용
        glm::vec3 baseUp = glm::vec3(0.0f, -1.0f, 0.0f);  // Vulkan 스타일
        glm::vec3 rotatedUp = additionalRotation * baseUp;

        // 4. 회전된 방향으로 뷰 설정
        setViewDirection(position, rotatedDirection, rotatedUp);
    }
}