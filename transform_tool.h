#pragma once

#include "lot_game_object.h"
#include "lot_camera.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <unordered_map>

namespace lot {

    class TransformTool {
    public:
        enum class Mode  { NONE, MOVE, ROTATE, SCALE };
        enum class State { IDLE, WAITING_BASE, PREVIEWING };

        // 키 입력 처리 (T/R/S/ESC)
        void handleKeyInput(GLFWwindow* window,
                            const LotCamera& camera,
                            LotGameObject::Map& gameObjects);

        // 마우스 좌클릭 처리 (기준점/목적지 클릭) - 매 프레임 호출
        bool handleMouseClick(GLFWwindow* window,
                              double mouseX, double mouseY,
                              int windowW, int windowH,
                              const LotCamera& camera,
                              LotGameObject::Map& gameObjects);

        // 매 프레임 미리보기 업데이트
        void updatePreview(double mouseX, double mouseY,
                           int windowW, int windowH,
                           const LotCamera& camera,
                           LotGameObject::Map& gameObjects);

        // ESC 취소 - 원본 transform 복원
        void cancel(LotGameObject::Map& gameObjects);

        bool isActive() const { return state != State::IDLE; }
        Mode getMode()  const { return mode; }
        State getState() const { return state; }
        const std::string& getNumberBuffer() const { return numberBuffer; }

    private:
        Mode  mode  { Mode::NONE };
        State state { State::IDLE };

        glm::vec3 basePoint{};      // 1번 클릭 지점
        float     baseAngle{};      // ROTATE용 초기 각도 (atan2)
        float     baseDist{};       // SCALE용 초기 거리

        // ESC 취소 대비 원본 transform 저장
        std::unordered_map<LotGameObject::id_t, Transformcomponent> savedTransforms;

        // 현재 뷰 기반 작업 평면
        glm::vec3 planeOrigin{};
        glm::vec3 planeNormal{};

        // 키 중복 방지
        bool tKeyWasPressed{ false };
        bool rKeyWasPressed{ false };
        bool sKeyWasPressed{ false };
        bool escKeyWasPressed{ false };

        // 마우스 좌클릭 중복 방지
        bool leftClickWasPressed{ false };

        // 숫자 입력 (PREVIEWING 상태)
        // MOVE: 이동 거리(world units), ROTATE: 각도(도), SCALE: 배율
        std::string numberBuffer{};
        glm::vec3   lastMouseWorldPos{};  // 마우스 방향 참조 (MOVE)
        bool digitKeyWasPressed[10]{};    // 0-9 (상단 숫자 키)
        bool kpDigitKeyWasPressed[10]{};  // 0-9 (넘패드)
        bool periodKeyWasPressed{ false };
        bool minusKeyWasPressed{ false };
        bool backspaceKeyWasPressed{ false };
        bool enterKeyWasPressed{ false };

        // 숫자 입력 확정 적용
        void applyNumberInput(LotGameObject::Map& gameObjects);

        // 현재 뷰 기준으로 작업 평면 설정
        void setupWorkPlane(const LotCamera& camera);

        // 마우스 → 작업 평면 위의 3D 좌표
        glm::vec3 getMouseWorldPos(double mouseX, double mouseY,
                                   int w, int h,
                                   const LotCamera& camera) const;

        // 선택된 오브젝트에 현재 모드 미리보기 적용
        void applyPreview(const glm::vec3& currentPoint,
                          LotGameObject::Map& gameObjects);

        // 확정 (savedTransforms 삭제)
        void confirm();

        // 변환 모드 진입
        void enterMode(Mode newMode, const LotCamera& camera,
                       LotGameObject::Map& gameObjects);
    };

} // namespace lot
