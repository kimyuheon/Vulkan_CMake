#pragma once

#include "lot_game_object.h"
#include "lot_window.h"

// GLM 쿼터니언 지원
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

namespace lot {
    class KeyboardMoveCtrl {
        public:
            struct KeyMappings {
                int moveLeft = GLFW_KEY_A;
                int moveRight = GLFW_KEY_D;
                int moveForward = GLFW_KEY_W;
                int moveBackward = GLFW_KEY_S;
                int moveUp = GLFW_KEY_Q;
                int moveDown = GLFW_KEY_E;

                int lookLeft = GLFW_KEY_LEFT;
                int lookRight = GLFW_KEY_RIGHT;
                int lookUp = GLFW_KEY_UP;
                int lookDown = GLFW_KEY_DOWN;

                // 객체 회전 (넘패드 - 새로 추가)
                int objRotateLeft = GLFW_KEY_KP_4;      // Y축 음수 회전
                int objRotateRight = GLFW_KEY_KP_6;     // Y축 양수 회전  
                int objRotateUp = GLFW_KEY_KP_8;        // X축 음수 회전
                int objRotateDown = GLFW_KEY_KP_2;      // X축 양수 회전
                int objRollLeft = GLFW_KEY_KP_7;        // Z축 음수 회전
                int objRollRight = GLFW_KEY_KP_9;       // Z축 양수 회전
                
                // 객체 선택
                int selectNextObject = GLFW_KEY_KP_ADD;
                int selectPrevObject = GLFW_KEY_KP_SUBTRACT;
                
                // 속도 조절
                int increaseRotSpeed = GLFW_KEY_KP_MULTIPLY;
                int decreaseRotSpeed = GLFW_KEY_KP_DIVIDE;
            };

            enum class ProjectionType {
                Orthographic,
                Perspective
            };

            // 생성자
            KeyboardMoveCtrl() {
                std::cout << "KeyboardMoveCtrl constructor called" << std::endl;
            }

            // 제어 함수
            void moveInPlaneXZ(GLFWwindow* window, float dt, LotGameObject& gameObject);
            void rotateObjects(GLFWwindow* window, float dt, std::vector<LotGameObject>& objects);
            void handleMouseCameraControl(GLFWwindow* window, float dt, LotGameObject& cameraObject, glm::vec3& targetPoint);
            // 투영 정보를 받는 새로운 함수 추가
            void handleMouseCameraControlWithProjection(
                GLFWwindow* window, 
                float dt, 
                LotGameObject& cameraObject, 
                glm::vec3& targetPoint,
                float orthoSize,
                float aspect
            );

            // 객체 회전 함수들
            void selectObject(size_t index, const std::vector<LotGameObject>& objects);
            void selectNextObject(const std::vector<LotGameObject>& objects);
            void selectPrevObject(const std::vector<LotGameObject>& objects);
            void rotateObjectsTest(GLFWwindow* window, float dt, LotGameObject& gameObject);

            // 현재 선택된 객체 정보
            size_t getSelectedObjectIndex() const { return selectedObjectIndex; }
            bool hasValidSelection() const { return hasValidObjectSelection; }
            LotGameObject* getSelectedObject(std::vector<LotGameObject>& objects);

            // 스크롤 관련 함수
            void processScrollInput(GLFWwindow* window, ProjectionType projType, 
                           float& orthoSize, LotGameObject& cameraObject, 
                           glm::vec3& targetPoint, float fov = glm::radians(50.0f));
            static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
            static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
            void setScrollDelta(double delta) { scrollDelta = delta; }
            void setMouseDelta(double deltaX, double deltaY) {
                mouseDeltaX = deltaX;
                mouseDeltaY = deltaY;
            }
            static void setInstance(KeyboardMoveCtrl* inst);

            KeyMappings keys{};
            float moveSpeed{3.f};
            float lookSpeed{1.5f};
            float objectRotationSpeed{2.0f};    // 객체 회전 속도
            float rotationSpeedIncrement{0.5f}; // 속도 증감량
            float maxRotationSpeed{10.0f};      // 최대 회전 속도
            float minRotationSpeed{0.1f};       // 최소 회전 속도

            // 마우스 제어 감도
            float mouseRotationSensitivity{0.005f}; // 원래 속도 복원
            float mouseMoveSensitivity{0.01f};      // Pan

            // 스크롤 
            float zoomSpeed{0.5f};
            float minOrthoSize{0.1f};
            float maxOrthoSize{10.0f};
        private:
            // 내부 헬퍼 함수들
            void handleKeyboardObjectControl(GLFWwindow* window, float dt, std::vector<LotGameObject>& gameObject);
            void processMouseInput(GLFWwindow* window);

            // 객체 선택 관련 건
            size_t selectedObjectIndex{0};
            bool hasValidObjectSelection{false};

            // 키 반복 입력 방지
            bool wasSelectNextPressed{false};
            bool wasSelectPrevPressed{false};
            bool wasIncreaseSpeedPressed{false};
            bool wasDecreaseSpeedPressed{false};

            // 마우스 상태 관리
            bool rightMousePressed{false};
            bool middleMousePressed{false};
            double lastMouseX{0.0};
            double lastMouseY{0.0};
            bool firstMouse{true};

            // Orbit 카메라 상태
            float orbitRadius{2.5f};      // 고정 반지름
            glm::mat3 cumulativeRotation{1.0f}; // 누적 회전 행렬 (CAD 스타일)
            bool orbitInitialized{false}; // 초기화 플래그

            // 쿼터니언 기반 회전 시스템 (짐벌락 해결)
            glm::quat orbitRotation{1.0f, 0.0f, 0.0f, 0.0f};  // 누적 회전 쿼터니언

            // 마우스 상태 추적
            bool wasRightPressed{false};
            bool wasMiddlePressed{false};
            int rightClickFrameCounter{0};  // 우클릭 후 프레임 카운터

            // 스크롤
            double scrollDelta{0.0};

            // 마우스 델타 (콜백용)
            double mouseDeltaX{0.0};
            double mouseDeltaY{0.0};

            static KeyboardMoveCtrl* instance;
    };
} // namespace lot