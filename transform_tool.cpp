#define GLM_ENABLE_EXPERIMENTAL
#include "transform_tool.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>
#include <iostream>

namespace lot {

    // ---------------------------------------------------------------
    // 모드 진입
    // ---------------------------------------------------------------
    void TransformTool::enterMode(Mode newMode, const LotCamera& camera,
                                  LotGameObject::Map& gameObjects) {
        // 선택된 오브젝트 확인
        bool hasSelected = false;
        for (auto& kv : gameObjects) {
            if (kv.second.isSelected && kv.second.model) {
                hasSelected = true;
                break;
            }
        }
        if (!hasSelected) return;

        mode  = newMode;
        state = State::WAITING_BASE;
        numberBuffer.clear();

        // 작업 평면 설정
        setupWorkPlane(camera);

        // 원본 transform 저장 (ESC 취소 대비)
        savedTransforms.clear();
        for (auto& kv : gameObjects) {
            if (kv.second.isSelected && kv.second.model) {
                savedTransforms[kv.first] = kv.second.transform;
            }
        }

        std::string modeName = (newMode == Mode::MOVE) ? "MOVE" :
                               (newMode == Mode::ROTATE) ? "ROTATE" : "SCALE";
        std::cout << "[TransformTool] " << modeName << " 모드 진입 - 기준점을 클릭하세요\n";
    }

    // ---------------------------------------------------------------
    // 작업 평면 설정 (현재 뷰 기준)
    // ---------------------------------------------------------------
    void TransformTool::setupWorkPlane(const LotCamera& camera) {
        // 평면 원점 = 카메라 타겟
        planeOrigin = camera.getTarget();

        switch (camera.getCurrentViewType()) {
            case LotCamera::CadViewType::Top:
                // XY 평면 (Z=고정), 회전축 = Z
                planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
                break;
            case LotCamera::CadViewType::Front:
                // XZ 평면 (Y=고정), 회전축 = Y
                planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                break;
            case LotCamera::CadViewType::Right:
                // YZ 평면 (X=고정), 회전축 = X
                planeNormal = glm::vec3(1.0f, 0.0f, 0.0f);
                break;
            case LotCamera::CadViewType::Isometric:
            default:
                // 기본: XY 평면
                planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
                break;
        }
    }

    // ---------------------------------------------------------------
    // 마우스 → 작업 평면 위의 3D 좌표 (sketch_manager와 동일 로직)
    // ---------------------------------------------------------------
    glm::vec3 TransformTool::getMouseWorldPos(double mouseX, double mouseY,
                                               int w, int h,
                                               const LotCamera& camera) const {
        float ndcX = static_cast<float>((2.0 * mouseX) / w - 1.0);
        float ndcY = static_cast<float>((2.0 * mouseY) / h - 1.0);

        glm::mat4 invProjView = glm::inverse(camera.getProjection() * camera.getView());

        glm::vec4 nearClip = glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
        glm::vec4 farClip  = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

        glm::vec4 worldNear = invProjView * nearClip;
        worldNear /= worldNear.w;
        glm::vec4 worldFar = invProjView * farClip;
        worldFar /= worldFar.w;

        glm::vec3 rayOrigin = glm::vec3(worldNear);
        glm::vec3 rayDir    = glm::normalize(glm::vec3(worldFar - worldNear));

        // 작업 평면과 교차점 계산
        float denom = glm::dot(rayDir, planeNormal);
        if (std::abs(denom) < 0.0001f) return planeOrigin;

        float t = glm::dot(planeOrigin - rayOrigin, planeNormal) / denom;
        if (t < 0.0f) return planeOrigin;

        return rayOrigin + rayDir * t;
    }

    // ---------------------------------------------------------------
    // 미리보기 적용
    // ---------------------------------------------------------------
    void TransformTool::applyPreview(const glm::vec3& currentPoint,
                                     LotGameObject::Map& gameObjects) {
        for (auto& kv : gameObjects) {
            if (!kv.second.isSelected || !kv.second.model) continue;
            auto it = savedTransforms.find(kv.first);
            if (it == savedTransforms.end()) continue;

            const Transformcomponent& orig = it->second;
            Transformcomponent& t = kv.second.transform;

            if (mode == Mode::MOVE) {
                glm::vec3 displacement = currentPoint - basePoint;
                t.translation = orig.translation + displacement;
            } else if (mode == Mode::ROTATE) {
                // 오브젝트 자신의 중심 기준 회전
                // basePoint(1번 클릭)는 각도 기준선, 객체 위치는 고정
                glm::vec3 objectCenter = orig.translation;

                glm::vec3 toBase    = basePoint    - objectCenter;
                glm::vec3 toCurrent = currentPoint - objectCenter;

                // 평면 위에서의 2D 각도 계산
                glm::vec3 axisX = glm::normalize(glm::cross(
                    glm::abs(planeNormal.z) < 0.9f ?
                    glm::vec3(0,0,1) : glm::vec3(1,0,0), planeNormal));
                glm::vec3 axisY = glm::cross(planeNormal, axisX);

                float baseA    = std::atan2(glm::dot(toBase,    axisY),
                                            glm::dot(toBase,    axisX));
                float currentA = std::atan2(glm::dot(toCurrent, axisY),
                                            glm::dot(toCurrent, axisX));
                float deltaAngle = currentA - baseA;

                glm::quat rotQ = glm::angleAxis(deltaAngle, planeNormal);

                // 위치는 유지, 회전만 적용
                t.translation = orig.translation;
                t.rotation    = glm::normalize(rotQ * orig.rotation);
            } else if (mode == Mode::SCALE) {
                // currentDist = 오브젝트 중심 ↔ 마우스 거리
                // baseDist    = 오브젝트 중심 ↔ basePoint 거리 (1번 클릭 시 고정)
                // → 마우스가 basePoint에 있을 때: currentDist == baseDist → factor == 1 (원본 크기)
                float currentDist = glm::length(currentPoint - orig.translation);
                if (baseDist > 0.001f) {
                    float factor = currentDist / baseDist;
                    factor = glm::clamp(factor, 0.01f, 100.0f);
                    t.scale = orig.scale * factor;
                    // basePoint를 앵커로 유지하면서 중심 이동
                    t.translation = basePoint + (orig.translation - basePoint) * factor;
                }
            }
        }
    }

    // ---------------------------------------------------------------
    // 확정
    // ---------------------------------------------------------------
    void TransformTool::confirm() {
        savedTransforms.clear();
        mode  = Mode::NONE;
        state = State::IDLE;
        std::cout << "[TransformTool] 확정 완료\n";
    }

    // ---------------------------------------------------------------
    // 취소 (ESC)
    // ---------------------------------------------------------------
    void TransformTool::cancel(LotGameObject::Map& gameObjects) {
        if (state == State::IDLE) return;

        // 원본 transform 복원
        for (auto& kv : savedTransforms) {
            auto it = gameObjects.find(kv.first);
            if (it != gameObjects.end()) {
                it->second.transform = kv.second;
            }
        }
        savedTransforms.clear();
        numberBuffer.clear();
        mode  = Mode::NONE;
        state = State::IDLE;
        std::cout << "[TransformTool] 취소됨\n";
    }

    // ---------------------------------------------------------------
    // 키 입력 처리
    // ---------------------------------------------------------------
    void TransformTool::handleKeyInput(GLFWwindow* window,
                                       const LotCamera& camera,
                                       LotGameObject::Map& gameObjects) {
        // T → MOVE
        bool tNow = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
        if (tNow && !tKeyWasPressed && state == State::IDLE) {
            enterMode(Mode::MOVE, camera, gameObjects);
        }
        tKeyWasPressed = tNow;

        // R → ROTATE
        bool rNow = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (rNow && !rKeyWasPressed && state == State::IDLE) {
            enterMode(Mode::ROTATE, camera, gameObjects);
        }
        rKeyWasPressed = rNow;

        // S → SCALE
        bool sNow = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        if (sNow && !sKeyWasPressed && state == State::IDLE) {
            enterMode(Mode::SCALE, camera, gameObjects);
        }
        sKeyWasPressed = sNow;

        // ESC → 취소
        bool escNow = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escNow && !escKeyWasPressed && state != State::IDLE) {
            cancel(gameObjects);
        }
        escKeyWasPressed = escNow;

        // ---------------------------------------------------------------
        // 숫자 입력 (PREVIEWING 상태에서만)
        // MOVE: 마우스 방향으로 N units 이동
        // ROTATE: N도 회전
        // SCALE: N배 축척
        // ---------------------------------------------------------------
        if (state == State::PREVIEWING) {
            // 0~9 (상단 키 / 넘패드)
            for (int i = 0; i <= 9; i++) {
                // 상단 키
                bool pressed = glfwGetKey(window, GLFW_KEY_0 + i) == GLFW_PRESS;
                if (pressed && !digitKeyWasPressed[i]) {
                    numberBuffer += static_cast<char>('0' + i);
                    std::cout << "[TransformTool] 입력: " << numberBuffer << "\n";
                }
                digitKeyWasPressed[i] = pressed;

                // 넘패드
                bool kpPressed = glfwGetKey(window, GLFW_KEY_KP_0 + i) == GLFW_PRESS;
                if (kpPressed && !kpDigitKeyWasPressed[i]) {
                    numberBuffer += static_cast<char>('0' + i);
                    std::cout << "[TransformTool] 입력: " << numberBuffer << "\n";
                }
                kpDigitKeyWasPressed[i] = kpPressed;
            }

            // 소수점 (중복 방지)
            bool periodPressed = glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS
                              || glfwGetKey(window, GLFW_KEY_KP_DECIMAL) == GLFW_PRESS;
            if (periodPressed && !periodKeyWasPressed
                && numberBuffer.find('.') == std::string::npos) {
                // 비워져있거나 '-'일 시 앞에 '0' 먼저 붙이기('.'전에)
                if (numberBuffer.empty() || numberBuffer == "-") numberBuffer += '0';
                numberBuffer += '.';
                std::cout << "[TransformTool] 입력: " << numberBuffer << "\n";
            }
            periodKeyWasPressed = periodPressed;

            // 마이너스 (첫 문자로만 허용)
            bool minusPressed = glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS
                             || glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS;
            if (minusPressed && !minusKeyWasPressed && numberBuffer.empty()) {
                numberBuffer += '-';
                std::cout << "[TransformTool] 입력: " << numberBuffer << "\n";
            }
            minusKeyWasPressed = minusPressed;

            // 백스페이스
            bool backPressed = glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
            if (backPressed && !backspaceKeyWasPressed && !numberBuffer.empty()) {
                numberBuffer.pop_back();
                std::cout << "[TransformTool] 입력: " << numberBuffer << "\n";
            }
            backspaceKeyWasPressed = backPressed;

            // Enter → 숫자 입력 확정
            bool enterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS
                             || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
            if (enterPressed && !enterKeyWasPressed && !numberBuffer.empty()) {
                applyNumberInput(gameObjects);
            }
            enterKeyWasPressed = enterPressed;
        }
    }

    // ---------------------------------------------------------------
    // 마우스 좌클릭 처리
    // returns true = TransformTool이 클릭을 소비 (선택 시스템으로 전달 안 함)
    // ---------------------------------------------------------------
    bool TransformTool::handleMouseClick(GLFWwindow* window,
                                         double mouseX, double mouseY,
                                         int windowW, int windowH,
                                         const LotCamera& camera,
                                         LotGameObject::Map& gameObjects) {
        if (state == State::IDLE) return false;

        bool leftNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (!leftNow || leftClickWasPressed) {
            leftClickWasPressed = leftNow;
            return (state != State::IDLE);
        }
        leftClickWasPressed = true;

        glm::vec3 worldPos = getMouseWorldPos(mouseX, mouseY, windowW, windowH, camera);

        if (state == State::WAITING_BASE) {
            basePoint = worldPos;

            if (mode == Mode::SCALE) {
                // baseDist = 오브젝트 중심 ↔ 클릭 위치 거리 (배율 기준선)
                // 마우스가 basePoint에 있을 때 factor=1 (원본 크기)
                baseDist = 1.0f;  // fallback
                for (auto& kv : gameObjects) {
                    if (kv.second.isSelected && kv.second.model) {
                        auto it = savedTransforms.find(kv.first);
                        if (it != savedTransforms.end()) {
                            float d = glm::length(basePoint - it->second.translation);
                            if (d > 0.001f) baseDist = d;
                            break;
                        }
                    }
                }
            }

            state = State::PREVIEWING;
            std::cout << "[TransformTool] 기준점 설정: ("
                      << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")\n";
        }
        else if (state == State::PREVIEWING) {
            // 최종 위치 확정
            applyPreview(worldPos, gameObjects);
            confirm();
        }

        return true;
    }

    // ---------------------------------------------------------------
    // 매 프레임 미리보기 업데이트
    // ---------------------------------------------------------------
    void TransformTool::updatePreview(double mouseX, double mouseY,
                                      int windowW, int windowH,
                                      const LotCamera& camera,
                                      LotGameObject::Map& gameObjects) {
        if (state != State::PREVIEWING) return;

        glm::vec3 currentPoint = getMouseWorldPos(mouseX, mouseY, windowW, windowH, camera);
        lastMouseWorldPos = currentPoint;

        applyPreview(currentPoint, gameObjects);
    }

    // ---------------------------------------------------------------
    // 숫자 입력 확정 적용
    // ---------------------------------------------------------------
    void TransformTool::applyNumberInput(LotGameObject::Map& gameObjects) {
        float value = 0.0f;
        try {
            value = std::stof(numberBuffer);
        } catch (...) {
            std::cout << "[TransformTool] 숫자 파싱 오류: '" << numberBuffer << "'\n";
            numberBuffer.clear();
            return;
        }
        numberBuffer.clear();
        std::cout << "[TransformTool] 숫자 입력 확정: " << value << "\n";

        for (auto& kv : gameObjects) {
            if (!kv.second.isSelected || !kv.second.model) continue;
            auto it = savedTransforms.find(kv.first);
            if (it == savedTransforms.end()) continue;

            const Transformcomponent& orig = it->second;
            Transformcomponent& t = kv.second.transform;

            if (mode == Mode::MOVE) {
                // 현재 마우스 방향으로 value 거리만큼 이동
                glm::vec3 dir = lastMouseWorldPos - basePoint;
                float len = glm::length(dir);
                if (len > 0.001f) {
                    t.translation = orig.translation + glm::normalize(dir) * value;
                } else {
                    std::cout << "[TransformTool] MOVE: 마우스 방향이 없습니다. 먼저 마우스를 이동하세요.\n";
                    return;
                }
            }
            else if (mode == Mode::ROTATE) {
                // value = 각도 (도 단위)
                float radians = glm::radians(value);
                glm::quat rotQ = glm::angleAxis(radians, planeNormal);
                t.translation = orig.translation;
                t.rotation    = glm::normalize(rotQ * orig.rotation);
            }
            else if (mode == Mode::SCALE) {
                // value = 배율 (0.5 = 절반, 2 = 2배)
                if (value <= 0.0001f) {
                    std::cout << "[TransformTool] SCALE: 0 이하의 배율은 허용되지 않습니다.\n";
                    return;
                }
                float factor = glm::clamp(value, 0.01f, 100.0f);
                t.scale = orig.scale * factor;
                t.translation = basePoint + (orig.translation - basePoint) * factor;
            }
        }

        confirm();
    }

} // namespace lot
