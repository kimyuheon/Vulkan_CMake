#pragma once

#include "lot_device.h"
#include "lot_game_object.h"
#include "lot_renderer.h"
#include "lot_window.h"
#include "object_selection_manager.h"
#include "keyboard_move_ctrl.h"
#include "lot_camera.h"
#include "simple_render_system.h"

#include <memory>
#include <vector>
#include <chrono>

#include <glm/glm.hpp>

namespace lot {
    class FirstApp {
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;

            FirstApp();
            ~FirstApp();

            FirstApp(const FirstApp &) = delete;
            FirstApp &operator=(const FirstApp&) = delete;

            void run();

        private:
            void loadGameObjects();
            void addNewCube();
            void removeSelectedObjects();

            // 메인 루프 함수들
            void updateCamera(KeyboardMoveCtrl& cameraCtrl, float frameTime,
                             LotGameObject& viewerObject, glm::vec3& orbitTarget,
                             KeyboardMoveCtrl::ProjectionType projectionType);
            void handleInputs(const std::chrono::high_resolution_clock::time_point& currentTime, const LotGameObject& viewerObject, LotCamera& camera);
            void updateProjection(LotCamera& camera, KeyboardMoveCtrl::ProjectionType projectionType, float aspect, const LotGameObject& viewerObject, const glm::vec3& orbitTarget);
            void handleResizing();
            void render(SimpleRenderSystem& renderSystem, LotCamera& camera);
            void printDebugInfo(const std::chrono::high_resolution_clock::time_point& currentTime,
                               const LotGameObject& viewerObject);

            LotWindow lotWindow{ WIDTH, HEIGHT, "Hellow Lot Vulkan!!!" };
            LotDevice lotDevice{ lotWindow };
            LotRenderer lotRenderer{ lotWindow, lotDevice };

            std::vector<LotGameObject> gameObjects;
            ObjectSelectionManager selectionManager;
    };

} // namespace lot