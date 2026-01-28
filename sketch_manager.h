#pragma once

#include "lot_game_object.h"
#include "lot_camera.h"
#include "lot_device.h"
#include "lot_model.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace lot {
    class SketchManager {
        public: 
            enum class SketchState {
                Idle,                  // 스케치 비활성
                WaitingFirstClick,     // 첫 클릭 대기 중
                WaitingSecondClick,    // 두 번째 클릭 대기 (사각형 확정)
                WaitingHeight          // 높이 조정 중
            };

            SketchManager(LotDevice& device);
            ~SketchManager() = default;

            // 업데이트 함수
            void handleInput(GLFWwindow* window, const LotCamera& camera, LotGameObject::Map& gameObjects);

            void startSketch();
            void cancelSketch();
            bool isSketchActive() const { return state != SketchState::Idle; }
            SketchState getState() { return state; }

            const std::vector<LotGameObject>& getPreviewObjects() const {
                return previewObjects;
            }
            void setCubeModel(std::shared_ptr<LotModel> model) { cubeModel = model; }
            LotGameObject::Map& getPreviewMap() { return previewMap; }
        private:
            glm::vec3 screenToWorldPlane(double mouseX, double mouseY, int windowWidth, int windowHeight,
                                        const LotCamera& camera, float planeHeight);
            // 프리뷰 객체 생성
            void updateRectanglePreview();
            void updateBoxPreview();
            // 최종 객체 생성
            LotGameObject createFinalBox(LotGameObject::Map& gameObjects);

            std::shared_ptr<LotModel> cubeModel;
            SketchState state = SketchState::Idle;

            glm::vec3 firstPoint{0.0f};
            glm::vec3 secondPoint{0.0f};
            glm::vec3 currentMousePos{0.0f};
            float baseHeight = 0.0f;

            std::vector<LotGameObject> previewObjects;
            LotGameObject::Map previewMap;
            LotDevice& lotDevice;

            // 마우스
            bool leftMousePressed = false;
            bool keyPressed = false;
            bool firstPointSet = false;

            // 높이 조절용
            float secondClickScreenY = 0.0f;  // 두 번째 클릭 시점 화면 Y
            float currentScreenY = 0.0f;      // 현재 화면 Y
            float heightScale = 0.02f;        // 화면 Y → 월드 높이 변환 스케일
    };
}