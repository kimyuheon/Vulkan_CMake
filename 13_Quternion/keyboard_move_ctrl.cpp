#include "keyboard_move_ctrl.h"
#include <iostream>

// GLM 실험적 확장 기능 활성화
#define GLM_ENABLE_EXPERIMENTAL

// GLM 쿼터니언 헤더 추가
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// std
#include <limits>

namespace lot {
    KeyboardMoveCtrl* KeyboardMoveCtrl::instance = nullptr;
    // 임시 함수
    void KeyboardMoveCtrl::rotateObjectsTest(GLFWwindow* window, float dt, LotGameObject& gameObject) {
        glm::vec3 rotationDelta{0.0f};

        // 넘패드 입력으로 회전
        if (glfwGetKey(window, keys.objRotateLeft) == GLFW_PRESS) { // Y축 왼쪽 회전
            rotationDelta.y -= objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRotateRight) == GLFW_PRESS) { // Y축 오른쪽 회전
            rotationDelta.y += objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRotateUp) == GLFW_PRESS) { // X축 위쪽 회전
            rotationDelta.x -= objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRotateDown) == GLFW_PRESS) { // X축 아래쪽 회전
            rotationDelta.x += objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRollLeft) == GLFW_PRESS) { // Z축 롤 왼쪽 회전
            rotationDelta.z -= objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRollRight) == GLFW_PRESS) { // Z축 롤 오른쪽 회전
            rotationDelta.z += objectRotationSpeed * dt;
        }

        // 회전 적용
        if (glm::dot(rotationDelta, rotationDelta) > std::numeric_limits<float>::epsilon()) {
            //gameObject.transform.rotation += rotationDelta;

            // 회전값 정규화 (0 ~ 360)
            if (rotationDelta.x != 0.0f) gameObject.transform.rotateAroundAxis(rotationDelta.x, glm::vec3(1, 0, 0));
            if (rotationDelta.y != 0.0f) gameObject.transform.rotateAroundAxis(rotationDelta.y, glm::vec3(0, 1, 0));
            if (rotationDelta.z != 0.0f) gameObject.transform.rotateAroundAxis(rotationDelta.z, glm::vec3(0, 0, 1));

            gameObject.transform.mat4();
        }
    }

    // 제어 함수
    void KeyboardMoveCtrl::moveInPlaneXZ(GLFWwindow* window, float dt, LotGameObject& gameObject) {
        glm::vec3 rotate{0};
        if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
        if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
        if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
        if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            std::cout << "*** moveInPlaneXZ CHANGING CAMERA - rotate: (" << rotate.x << ", " << rotate.y << ", " << rotate.z << ") ***" << std::endl;
            //gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
            glm::vec3 normalizedRotate = glm::normalize(rotate);
            if (normalizedRotate.x != 0.0f) gameObject.transform.rotateAroundAxis(lookSpeed * dt * normalizedRotate.x, glm::vec3(1, 0, 0));
            if (normalizedRotate.y != 0.0f) gameObject.transform.rotateAroundAxis(lookSpeed * dt * normalizedRotate.y, glm::vec3(0, 1, 0));
        }

        // gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);  // 무제한 회전 허용
        //gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

        // 쿼터니언에서 직접 방향 벡터 계산
        glm::mat3 rotMatrix = glm::mat3_cast(gameObject.transform.rotation);
        const glm::vec3 forwardDir = rotMatrix * glm::vec3(0.0f, 0.0f, 1.0f);  // 로컬 Z축
        const glm::vec3 rightDir = rotMatrix * glm::vec3(1.0f, 0.0f, 0.0f);    // 로컬 X축  
        const glm::vec3 upDir = rotMatrix * glm::vec3(0.0f, -1.0f, 0.0f);      // 로컬 Y축

        glm::vec3 moveDir{0.f};
        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
        if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
        if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.translation += moveSpeed * dt *glm::normalize(moveDir);
        }
    }

    void KeyboardMoveCtrl::rotateObjects(GLFWwindow* window, float dt, std::vector<LotGameObject>& objects) {
        if (objects.empty()) {
            hasValidObjectSelection = false;
            return;
        }

        // 객체 선택 처리
        bool selectNextPressed = glfwGetKey(window, keys.selectNextObject) == GLFW_PRESS;
        bool selectPrevPressed = glfwGetKey(window, keys.selectPrevObject) == GLFW_PRESS;

        if (selectNextPressed && !wasSelectNextPressed) {
            selectNextObject(objects);
        }
        if (selectPrevPressed && !wasSelectPrevPressed) {
            selectPrevObject(objects);
        }

        wasSelectNextPressed = selectNextPressed;
        wasSelectPrevPressed = selectPrevPressed;

        // 회전 속도 조절
        bool increaseSpeedPressed = glfwGetKey(window, keys.increaseRotSpeed) == GLFW_PRESS;
        bool decreaseSpeedPressed = glfwGetKey(window, keys.decreaseRotSpeed) == GLFW_PRESS;

        if (increaseSpeedPressed && !wasIncreaseSpeedPressed) {
            objectRotationSpeed = std::min(objectRotationSpeed + rotationSpeedIncrement, maxRotationSpeed);
            std::cout << "Object rotation speed : " << objectRotationSpeed << std::endl;
        }
        if (decreaseSpeedPressed && !wasDecreaseSpeedPressed) {
            objectRotationSpeed = std::max(objectRotationSpeed - rotationSpeedIncrement, minRotationSpeed);
            std::cout << "Object rotation speed : " << objectRotationSpeed << std::endl;
        }

        wasIncreaseSpeedPressed = increaseSpeedPressed;
        wasDecreaseSpeedPressed = decreaseSpeedPressed;

        // 선택된 객체 없을 시 첫번째 객체 자동 선택
        if (!hasValidObjectSelection) {
            selectObject(0, objects);
        }

        // 회전 입력 처리
        handleKeyboardObjectControl(window, dt, objects);
    }

    void KeyboardMoveCtrl::handleMouseCameraControl(GLFWwindow* window, float dt, LotGameObject& cameraObject, glm::vec3& targetPoint) {
        // 이 함수 완전 비활성화 - 다른 곳에서 호출되면 문제가 됨
        std::cout << "*** WARNING: handleMouseCameraControl called - this should not be used ***" << std::endl;
        return;

        double currentMouseX, currentMouseY;
        glfwGetCursorPos(window, &currentMouseX, &currentMouseY);

        if (firstMouse) {
            lastMouseX = currentMouseX;
            lastMouseY = currentMouseY;
            firstMouse = false;
        }

        // 마우스 이동량 계산
        double deltaX = currentMouseX - lastMouseX;
        double deltaY = currentMouseY - lastMouseY;

        if (rightMousePressed) {
            // 벡터(현재 카메라 - 타겟)
            glm::vec3 currentOffset = cameraObject.transform.translation - targetPoint;
            float radius = glm::length(currentOffset);

            // 구면 좌표계 계산
            float currentView = atan2(currentOffset.z, currentOffset.x);
            float currentPitch = asin(glm::clamp(currentOffset.y / radius, -1.0f, 1.0f));

            // 마우스 이동량 -> 각도로 변환
            float deltaYaw = -static_cast<float>(deltaX) * mouseRotationSensitivity;
            float deltaPitch = -static_cast<float>(deltaY) * mouseRotationSensitivity;

            // 새로운 각도 계산 (피치 제한)
            float newYaw = currentView + deltaYaw;
            float newPitch = currentPitch + deltaPitch;  // 무제한 상하 회전

            // 새로운 카메라 위치 계산 (구면 좌표 -> 직교 좌표)
            glm::vec3 newOffset = glm::vec3(radius * cos(newPitch) * cos(newYaw),
                                            radius * sin(newPitch), 
                                            radius * cos(newPitch) * sin(newYaw));

            cameraObject.transform.translation = targetPoint + newOffset;

            // 쿼터니언으로 회전 설정 (짐벌락 해결)
            glm::quat yawQuat = glm::angleAxis(newYaw, glm::vec3(0, 1, 0));
            glm::quat pitchQuat = glm::angleAxis(newPitch, glm::vec3(1, 0, 0));
            cameraObject.transform.rotation = yawQuat * pitchQuat;            
        }

        // 휠 클릭 드래그
        if (middleMousePressed) {
            // 화면 크기 정보 가져오기
            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            
            // 직교 투영 정보 (first_app.cpp에서 설정한 값과 일치해야 함)
            float orthoSize = 2.0f;
            float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
            
            // 화면 좌표를 월드 좌표로 변환하는 스케일 계산
            float worldWidth = orthoSize * aspect * 2.0f;   // 전체 가로 크기
            float worldHeight = orthoSize * 2.0f;           // 전체 세로 크기
            
            // 픽셀당 월드 단위 계산
            float pixelToWorldX = worldWidth / windowWidth;
            float pixelToWorldY = worldHeight / windowHeight;
            
            // 카메라의 현재 방향 벡터들 계산
            glm::vec3 forward = glm::normalize(targetPoint - cameraObject.transform.translation);
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            glm::vec3 up = glm::normalize(glm::cross(right, forward));
            
            // 마우스 이동을 월드 좌표로 변환
            float worldDeltaX = static_cast<float>(deltaX) * pixelToWorldX;
            float worldDeltaY = static_cast<float>(-deltaY) * pixelToWorldY;  // Y축 반전
            
            // 정확한 Pan 계산
            glm::vec3 panDelta = right * worldDeltaX + up * worldDeltaY;
            
            cameraObject.transform.translation += panDelta;
            targetPoint += panDelta;
        }

        // 마지막 위치 업데이트 (항상 업데이트하여 다음 프레임에서 올바른 델타 계산)
        lastMouseX = currentMouseX;
        lastMouseY = currentMouseY;
    }

    void KeyboardMoveCtrl::handleMouseCameraControlWithProjection(GLFWwindow* window, float dt,
                                                                  LotGameObject& cameraObject,
                                                                  glm::vec3& targetPoint,
                                                                  float orthoSize,
                                                                  float aspect) {
        // 마우스 입력 처리
        processMouseInput(window);

        double currentMouseX, currentMouseY;
        glfwGetCursorPos(window, &currentMouseX, &currentMouseY);

        if (firstMouse) {
            lastMouseX = currentMouseX;
            lastMouseY = currentMouseY;
            firstMouse = false;
        }

        // 마우스 이동량 계산
        double deltaX = currentMouseX - lastMouseX;
        double deltaY = currentMouseY - lastMouseY;

        // 우클릭 중이 아닐 때는 델타를 0으로 설정
        //if (!rightMousePressed) {
        //    deltaX = 0.0;
        //    deltaY = 0.0;
        //}

        // 디버그: 마우스 위치와 이동량 출력 (더 자세히)
        static double prevMouseX = -1, prevMouseY = -1;
        if (rightMousePressed) {
            bool posChanged = (currentMouseX != prevMouseX || currentMouseY != prevMouseY);
            std::cout << "Mouse pos: (" << currentMouseX << ", " << currentMouseY << "), "
                     << "Last: (" << lastMouseX << ", " << lastMouseY << "), "
                     << "Delta: (" << deltaX << ", " << deltaY << "), "
                     << "PosChanged: " << posChanged << std::endl;
        }
        prevMouseX = currentMouseX;
        prevMouseY = currentMouseY;

        if (rightMousePressed) {
            // 현재 위치에서 타겟까지의 벡터
            glm::vec3 currentOffset = cameraObject.transform.translation - targetPoint;
            float currentRadius = glm::length(currentOffset);

            // 안전장치
            if (currentRadius < 0.001f) {
                return;
            }

            // 초기화 (radius만 설정, 회전은 현재 카메라 상태 유지)
            if (!orbitInitialized) {
                orbitRadius = currentRadius;
                orbitInitialized = true;

                // 기존 orbitRotation 초기화는 제거 - 새로운 시스템은 transform.rotation 직접 사용
            }

            // === 3DS Max 스타일 자유 회전 ===
            if (abs(deltaX) > 0.001 || abs(deltaY) > 0.001) {
                // 마우스 이동량을 회전각으로 변환
                float yawDelta = static_cast<float>(deltaX) * mouseRotationSensitivity;
                float pitchDelta = -static_cast<float>(deltaY) * mouseRotationSensitivity;

                // 수직 회전 (X축 기준) - 로컬 회전
                if (abs(pitchDelta) > 0.001f) {
                    cameraObject.transform.rotateAroundAxis(pitchDelta, glm::vec3(1, 0, 0));
                }

                // 수평 회전 (Y축 기준) - 월드 회전
                if (abs(yawDelta) > 0.001f) {
                    glm::quat worldYaw = glm::angleAxis(yawDelta, glm::vec3(0, 1, 0));
                    cameraObject.transform.rotation = worldYaw * cameraObject.transform.rotation;
                }

                // 타겟 주위로 공전 (카메라를 타겟에서 일정 거리 유지)
                glm::mat3 rotMatrix = glm::mat3_cast(cameraObject.transform.rotation);
                glm::vec3 forward = rotMatrix * glm::vec3(0.0f, 0.0f, 1.0f);
                cameraObject.transform.translation = targetPoint - forward * orbitRadius;
            }
        }

        // 개선된 Pan 로직 - 이제 orthoSize와 aspect를 매개변수로 받음
        if (middleMousePressed) {
            // 화면 크기 정보 가져오기
            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            
            // 매개변수로 받은 투영 정보 사용
            // 월드 공간 크기 계산
            float worldWidth = orthoSize * aspect * 2.0f;   // 전체 가로 크기
            float worldHeight = orthoSize * 2.0f;           // 전체 세로 크기
            
            // 픽셀당 월드 단위 계산
            float pixelToWorldX = worldWidth / windowWidth;
            float pixelToWorldY = worldHeight / windowHeight;
            
            // 카메라의 현재 방향 벡터들 계산
            glm::vec3 forward = glm::normalize(targetPoint - cameraObject.transform.translation);
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            glm::vec3 up = glm::normalize(glm::cross(right, forward));
            
            // 마우스 이동을 월드 좌표로 변환
            float worldDeltaX = static_cast<float>(deltaX) * pixelToWorldX;
            float worldDeltaY = static_cast<float>(-deltaY) * pixelToWorldY;  // Y축 반전
            
            // 정확한 Pan 계산 - 이제 orthoSize가 바뀌어도 정확히 작동
            glm::vec3 panDelta = right * worldDeltaX + up * worldDeltaY;
            
            cameraObject.transform.translation += panDelta;
            targetPoint += panDelta;
        }

        // 마지막 위치 업데이트 (항상 업데이트하여 다음 프레임에서 올바른 델타 계산)
        lastMouseX = currentMouseX;
        lastMouseY = currentMouseY;
    }

    // 스크롤 콜백 함수 (static)
    void KeyboardMoveCtrl::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        if (instance) {
            instance->setScrollDelta(yoffset);
        }
    }

    // 마우스 콜백 함수 (static) - 비활성화 (glfwGetCursorPos 사용)
    void KeyboardMoveCtrl::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        // 콜백 방식 비활성화 - 직접 glfwGetCursorPos 사용
        return;
    }

    void KeyboardMoveCtrl::processScrollInput(GLFWwindow* window, ProjectionType projType,
                                         float& orthoSize, LotGameObject& cameraObject,
                                         glm::vec3& targetPoint, float fov) {
        if (abs(scrollDelta) < 0.001) return;  // 스크롤이 없으면 리턴
        std::cout << "*** processScrollInput called - scrollDelta: " << scrollDelta << " ***" << std::endl;
        
        switch (projType) {
            case ProjectionType::Orthographic: {
                // Orthographic: orthoSize 조절
                float zoomFactor = static_cast<float>(scrollDelta) * zoomSpeed;
                orthoSize -= zoomFactor;  // 스크롤 업 = 확대 (orthoSize 감소)
                orthoSize = glm::clamp(orthoSize, minOrthoSize, maxOrthoSize);
                break;
            }
            
            case ProjectionType::Perspective: {
                // Perspective: 카메라 거리 조절
                glm::vec3 offset = cameraObject.transform.translation - targetPoint;
                float currentDistance = glm::length(offset);

                float zoomFactor = static_cast<float>(scrollDelta) * zoomSpeed * currentDistance * 0.1f;
                float newDistance = currentDistance - zoomFactor;
                newDistance = glm::clamp(newDistance, 0.5f, 50.0f);  // 거리 제한

                if (currentDistance > 0.001f) {  // 0으로 나누기 방지
                    glm::vec3 direction = glm::normalize(offset);
                    cameraObject.transform.translation = targetPoint + direction * newDistance;

                    // Perspective일 때 virtual orthoSize도 업데이트
                    orthoSize = newDistance * tan(fov * 0.5f);

                    // orbitRadius도 업데이트하여 다음 회전 시 뷰가 점프하지 않도록 함
                    orbitRadius = newDistance;
                }
                break;
            }
        }
        
        scrollDelta = 0.0;  // 스크롤 델타 리셋
    }

    void KeyboardMoveCtrl::setInstance(KeyboardMoveCtrl* inst) {
        instance = inst;
    }

    // 내부 헬퍼 함수
    void KeyboardMoveCtrl::handleKeyboardObjectControl(GLFWwindow* window, float dt, std::vector<LotGameObject>& gameObject) {
        if (!hasValidObjectSelection || selectedObjectIndex >= gameObject.size()) {
            return;
        }

        auto& selectedObject = gameObject[selectedObjectIndex];
        glm::vec3 rotationDelta{0.0f};        

        // 넘패드 입력으로 회전
        if (glfwGetKey(window, keys.objRotateLeft) == GLFW_PRESS) { // Y축 왼쪽 회전
            rotationDelta.y -= objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRotateRight) == GLFW_PRESS) { // Y축 오른쪽 회전
            rotationDelta.y += objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRotateUp) == GLFW_PRESS) { // Y축 위쪽 회전
            rotationDelta.x -= objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRotateDown) == GLFW_PRESS) { // Y축 아래쪽 회전
            rotationDelta.x += objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRollLeft) == GLFW_PRESS) { // Z축 롤 왼쪽 회전
            rotationDelta.z -= objectRotationSpeed * dt;
        }
        if (glfwGetKey(window, keys.objRollRight) == GLFW_PRESS) { // Z축 롤 오른쪽 회전
            rotationDelta.z += objectRotationSpeed * dt;
        }

        // 회전 적용
        if (glm::dot(rotationDelta, rotationDelta) > std::numeric_limits<float>::epsilon()) {

            // 회전값 정규화 (0 ~ 360)
            if (rotationDelta.x != 0.0f) selectedObject.transform.rotateAroundAxis(rotationDelta.x, glm::vec3(1, 0, 0));
            if (rotationDelta.y != 0.0f) selectedObject.transform.rotateAroundAxis(rotationDelta.y, glm::vec3(0, 1, 0));
            if (rotationDelta.z != 0.0f) selectedObject.transform.rotateAroundAxis(rotationDelta.z, glm::vec3(0, 0, 1));
        }        
    }

    void KeyboardMoveCtrl::processMouseInput(GLFWwindow* window) {
        int rightMouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

        rightMousePressed = (rightMouseState == GLFW_PRESS);

        int middleMouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
        middleMousePressed = (middleMouseState == GLFW_PRESS);

        wasRightPressed = rightMousePressed;
        wasMiddlePressed = middleMousePressed;
    }

    // 객체 선택 함수
    void KeyboardMoveCtrl::selectObject(size_t index, const std::vector<LotGameObject>& objects) {
        if (index < objects.size()) {
            selectedObjectIndex = index;
            hasValidObjectSelection = true;
            //std::cout << "Selected Object " << index << " (ID:" << objects[index].getId() << ")" << std::endl;
        } else {
            hasValidObjectSelection = false;
        }
    }

    void KeyboardMoveCtrl::selectNextObject(const std::vector<LotGameObject>& objects) {
        if (objects.empty()) {
            hasValidObjectSelection = false;
            return;
        }

        size_t nextIndex = hasValidObjectSelection ? (selectedObjectIndex + 1) % objects.size() : 0;
        selectObject(nextIndex, objects);
    }

    void KeyboardMoveCtrl::selectPrevObject(const std::vector<LotGameObject>& objects) {
        if (objects.empty()) {
            hasValidObjectSelection = false;
            return;
        }

        size_t prevIndex = hasValidObjectSelection ? 
            (selectedObjectIndex == 0 ? objects.size() - 1 : selectedObjectIndex - 1) : 0;
        selectObject(prevIndex, objects);
    }

    LotGameObject* KeyboardMoveCtrl::getSelectedObject(std::vector<LotGameObject>& objects) {
        if (hasValidObjectSelection && selectedObjectIndex < objects.size()) {
            return &objects[selectedObjectIndex];
        }
        return nullptr;
    }
} // namespace lot