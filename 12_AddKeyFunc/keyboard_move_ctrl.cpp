#include "keyboard_move_ctrl.h"
#include <iostream>


// std
#include <limits>

namespace lot {
    void KeyboardMoveCtrl::moveInPlaneXZ(GLFWwindow* window, float dt, LotGameObject& gameObject) {
        glm::vec3 rotate{0};
        if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
        if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
        if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
        if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.rotation += lookSpeed * dt *glm::normalize(rotate);
        }

        gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
        gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

        float yaw = gameObject.transform.rotation.y;
        const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
        const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
        const glm::vec3 upDir{0.f, -1.f, 0.f};

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
            gameObject.transform.rotation += rotationDelta;

            // 회전값 정규화 (0 ~ 360)
            gameObject.transform.rotation.x = glm::mod(gameObject.transform.rotation.x, glm::two_pi<float>());
            gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
            gameObject.transform.rotation.z = glm::mod(gameObject.transform.rotation.z, glm::two_pi<float>());

            gameObject.transform.mat4();
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
        if (hasValidObjectSelection && selectedObjectIndex < objects.size()) {
            auto& selectedObject = objects[selectedObjectIndex];
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
                selectedObject.transform.rotation += rotationDelta;

                // 회전값 정규화 (0 ~ 360)
                selectedObject.transform.rotation.x = glm::mod(selectedObject.transform.rotation.x, glm::two_pi<float>());
                selectedObject.transform.rotation.y = glm::mod(selectedObject.transform.rotation.y, glm::two_pi<float>());
                selectedObject.transform.rotation.z = glm::mod(selectedObject.transform.rotation.z, glm::two_pi<float>());
            }
        }
    }

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