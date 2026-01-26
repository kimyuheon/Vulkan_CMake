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
    
    // CAD 관련
    void LotCamera::orbitAroundTarget(float deltaX, float deltaY) {
        glm::mat4 rotMatrix4 = glm::mat4_cast(orbitRotation);
        glm::mat3 rotMatrix = glm::mat3(rotMatrix4);

        glm::vec3 right = rotMatrix * glm::vec3(1, 0, 0);
        glm::vec3 up = rotMatrix * glm::vec3(0, 1, 0);

        glm::quat pitchQuat = glm::angleAxis(deltaY, right);
        glm::quat yawQuat = glm::angleAxis(deltaX, up);

        orbitRotation = glm::normalize(yawQuat * pitchQuat * orbitRotation);

        updateCadView();
    }

    void LotCamera::zoomToTarget(float delta, bool isOrtho, glm::vec3* mouseWorldPos) {
        if (isOrtho) {
            float oldSize = orthographicSize;
            orthographicSize -= delta * 0.5f;
            orthographicSize = glm::clamp(orthographicSize, 0.1f, 50.0f);

            if (mouseWorldPos) {
                // 마우스 위치를 타겟 평면에 투영
                glm::mat3 rotMatrix = glm::mat3(glm::mat4_cast(orbitRotation));
                glm::vec3 forward = rotMatrix * glm::vec3(0.0f, 0.0f, 1.0f);

                // 마우스 레이와 타겟 평면의 교차점 계산
                float t = glm::dot(targetPosition - *mouseWorldPos, forward) / glm::dot(forward, forward);
                glm::vec3 mouseOnPlane = *mouseWorldPos + forward * t;

                float zoomRatio = orthographicSize / oldSize;
                glm::vec3 toMouse = mouseOnPlane - targetPosition;
                targetPosition += toMouse * (1.0f - zoomRatio);
            }
        } else {
            float oldDistance = orbitDistance;
            orbitDistance -= delta;
            orbitDistance = glm::max(orbitDistance, 0.5f);
            if (mouseWorldPos) {
                // 마우스 위치를 타겟 평면에 투영
                glm::mat3 rotMatrix = glm::mat3(glm::mat4_cast(orbitRotation));
                glm::vec3 forward = rotMatrix * glm::vec3(0.0f, 0.0f, 1.0f);

                // 마우스 레이와 타겟 평면의 교차점 계산
                float t = glm::dot(targetPosition - *mouseWorldPos, forward) / glm::dot(forward, forward);
                glm::vec3 mouseOnPlane = *mouseWorldPos + forward * t;

                float zoomRatio = orbitDistance / oldDistance;
                glm::vec3 toMouse = mouseOnPlane - targetPosition;
                targetPosition += toMouse * (1.0f - zoomRatio);
            }
        }
        updateCadView();
    }

    void LotCamera::panTarget(float deltaX, float deltaY, bool isOrtho, float aspect) {
        glm::mat3 roMatrix = glm::mat3(glm::mat4_cast(orbitRotation));

        glm::vec3 right = roMatrix * glm::vec3(1.0f, 0.0f, 0.0f); // 로컬 X
        glm::vec3 up = roMatrix * glm::vec3(0.0f, -1.0f, 0.0f);   // 로컬 Y

        if (isOrtho) {
            targetPosition += right * deltaX * orthographicSize * aspect + 
                              up * deltaY * orthographicSize;
        } else {
            float scale = (orbitDistance) * glm::tan(glm::radians(25.0f));
            targetPosition += (right * deltaX + up * deltaY) * scale;
        }

        updateCadView();
    }

    void LotCamera::setTarget(glm::vec3 newTarget) {
        targetPosition = newTarget;
        updateCadView();
    }

    void LotCamera::updateCadView() {
        glm::mat4 rotMatrix4 = glm::mat4_cast(orbitRotation);
        glm::mat3 rotMatrix = glm::mat3(rotMatrix4);

        // 기본 방향 벡터(카메라 -Z 방향)
        glm::vec3 defaultForward = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 defaultUp = glm::vec3(0.0f, -1.0f, 0.0f);

        // 회전 적용된 방향 벡터
        glm::vec3 forward = rotMatrix * defaultForward;
        glm::vec3 up = rotMatrix * defaultUp;

        // 타켓으로부터 orbitDistance 만큼 위치 계산
        glm::vec3 cameraPosition = targetPosition - forward * orbitDistance;

        // 카메라가 타겟을 바라보도록 설정
        setViewTarget(cameraPosition, targetPosition, up);
    }

    void LotCamera::resetCadRotation(CadViewType viewType) {
        
        orbitDistance = 10.0f;  // 초기 거리로 리셋
        targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);  // 원점으로 리셋

        switch (viewType)
        {
        case CadViewType::Top:
            // Top View (평면도) - X축 기준 -90도 회전하여 위에서 아래를 내려다봄
            orbitRotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            break;
        case CadViewType::Front:
            // Front View (정면도) - 기본 방향
            orbitRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            break;
        case CadViewType::Right:
            // Right View (우측면도) - Y -90도
            orbitRotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        case CadViewType::Isometric:
        {
            // 입체뷰
            glm::quat rotX = glm::angleAxis(glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            glm::quat rotY = glm::angleAxis(glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            orbitRotation = rotY * rotX;
            break;
        }
        default:
            break;
        }

        updateCadView();
    }
}