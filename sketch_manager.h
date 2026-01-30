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

            enum class SketchPlane {
                XY,
                YZ,
                XZ
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
            glm::vec3 getMouseWorldPosition(double mouseX, double mouseY, int windowWidth, int windowHeight,
                                            const LotCamera& camera, bool constrainToPlane);
            glm::vec3 screenToWorldPlane(double mouseX, double mouseY, int windowWidth, int windowHeight,
                                        const LotCamera& camera, float planeHeight);
            // 프리뷰 객체 생성
            void updateRectanglePreview();
            void updateBoxPreview();
            // 최종 객체 생성
            LotGameObject createFinalBox(LotGameObject::Map& gameObjects);

            SketchPlane determineSketchPlane(const LotCamera& camera);
            void getPlaneVectors(SketchPlane plane, glm::vec3& right, glm::vec3& up, glm::vec3& normal);
            void initializeSketchPlane(const LotCamera& camera);

            // 좌표변환 함수
            glm::vec3 localToWorld(const glm::vec2& localPoint, float depth = 0.0f);
            glm::vec2 WorldToLocal(const glm::vec3& worldPoint);

            std::shared_ptr<LotModel> cubeModel;
            SketchState state = SketchState::Idle;

            glm::vec3 firstPoint{0.0f};
            glm::vec3 secondPoint{0.0f};
            glm::vec3 currentMousePos{0.0f};
            //float baseHeight = 0.0f;

            std::vector<LotGameObject> previewObjects;
            LotGameObject::Map previewMap;
            LotDevice& lotDevice;

            // 마우스
            bool leftMousePressed = false;
            bool keyPressed = false;
            bool firstPointSet = false;

            // 높이 조절용
            glm::vec3 heightStartPos{0.0f};
            double heightStartScreenY = 0.0;   // 두 번째 클릭 시점 화면 Y
            double currentScreenY = 0.0;       // 현재 화면 Y
            float heightScale = 0.01f;         // 화면 Y → 월드 높이 변환 스케일

            // 스케치 평면 좌표계
            SketchPlane activeSketchPlane;
            glm::vec3 sketchPlaneOrigin{0.0f};
            glm::vec3 sketchPlaneRight{1.0f, 0.0f, 0.0f};
            glm::vec3 sketchPlaneUp{0.0f, 1.0f, 0.0f};
            glm::vec3 sketchPlaneNormal{0.0f, 0.0f, 1.0f};

            // 초기화 플래그
            LotCamera::CadViewType  sketchViewType;
            bool viewTypeInitialized = false;
    };
}