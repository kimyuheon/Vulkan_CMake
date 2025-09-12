#pragma once

#include "lot_game_object.h"
#include "lot_window.h"

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

            void moveInPlaneXZ(GLFWwindow* window, float dt, LotGameObject& gameObject);

            // 객체 회전 함수들
            void rotateObjects(GLFWwindow* window, float dt, std::vector<LotGameObject>& objects);
            void selectObject(size_t index, const std::vector<LotGameObject>& objects);
            void selectNextObject(const std::vector<LotGameObject>& objects);
            void selectPrevObject(const std::vector<LotGameObject>& objects);
            void rotateObjectsTest(GLFWwindow* window, float dt, LotGameObject& gameObject);

            // 현재 선택된 객체 정보
            size_t getSelectedObjectIndex() const { return selectedObjectIndex; }
            bool hasValidSelection() const { return hasValidObjectSelection; }
            LotGameObject* getSelectedObject(std::vector<LotGameObject>& objects);

            KeyMappings keys{};
            float moveSpeed{3.f};
            float lookSpeed{1.5f};
            float objectRotationSpeed{2.0f};    // 객체 회전 속도
            float rotationSpeedIncrement{0.5f}; // 속도 증감량
            float maxRotationSpeed{10.0f};      // 최대 회전 속도
            float minRotationSpeed{0.1f};       // 최소 회전 속도
        private:
            // 객체 선택 관련 건
            size_t selectedObjectIndex{0};
            bool hasValidObjectSelection{false};

            // 키 반복 입력 방지
            bool wasSelectNextPressed{false};
            bool wasSelectPrevPressed{false};
            bool wasIncreaseSpeedPressed{false};
            bool wasDecreaseSpeedPressed{false};
    };
} // namespace lot