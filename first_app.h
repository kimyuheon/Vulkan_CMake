#pragma once

#include "lot_device.h"
#include "lot_game_object.h"
#include "lot_renderer.h"
#include "lot_window.h"
#include "object_selection_manager.h"
#include "keyboard_move_ctrl.h"
#include "lot_camera.h"
#include "simple_render_system.h"
#include "point_light_system.h"
#include "lot_buffer.h"
#include "lot_descriptors.h"

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
            void handleInputs(const std::chrono::high_resolution_clock::time_point& currentTime, 
                              LotGameObject& viewerObject, LotCamera& camera, KeyboardMoveCtrl& cameraCtrl,
                              bool& enableLighting, 
                              bool& gKeyPressed, KeyboardMoveCtrl::ProjectionType& projectionType,
                              float frameTime, glm::vec3& orbitTarget);
            void updateProjection(LotCamera& camera, KeyboardMoveCtrl::ProjectionType projectionType, float aspect, 
                                  const LotGameObject& viewerObject, const glm::vec3& orbitTarget);
            void handleResizing();
            //void render(SimpleRenderSystem& renderSystem, LotCamera& camera, bool enableLighting);
            void printDebugInfo(const std::chrono::high_resolution_clock::time_point& currentTime,
                               const LotGameObject& viewerObject, bool enableLighting);

            LotWindow lotWindow{ WIDTH, HEIGHT, "Hellow Lot Vulkan!!!" };
            LotDevice lotDevice{ lotWindow };
            LotRenderer lotRenderer{ lotWindow, lotDevice };

            std::unique_ptr<LotDescriptorPool> globalPool{};
            std::unique_ptr<LotDescriptorSetLayout> globalSetLayout{};
            std::vector<std::unique_ptr<LotBuffer>> uboBuffers;
            std::vector<VkDescriptorSet> globalDescriptorSets;

            std::unique_ptr<SimpleRenderSystem> simpleRenderSystem;
            std::unique_ptr<PointLightSystem> pointLightSystem;
            LotGameObject::Map gameObjects;
            ObjectSelectionManager selectionManager;

            float orthoSize{5.0f};
            std::string WinTitleStr;
    };

} // namespace lot