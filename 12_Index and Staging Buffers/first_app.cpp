#include "first_app.h"

#include "keyboard_move_ctrl.h"
#include "lot_camera.h"
#include "simple_render_system.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <stdexcept>

#include <thread>

namespace lot {
    struct SimplePushConstantData {
        glm::mat2 transform{1.f};
        glm::vec2 offset;
        alignas(16) glm::vec3 color;
    };

    FirstApp::FirstApp() { loadGameObjects(); }

    FirstApp::~FirstApp() {
        vkDeviceWaitIdle(lotDevice.device());
    }

    void FirstApp::run() {
        SimpleRenderSystem simpleRenderSystem{ lotDevice, lotRenderer.getSwapChainRenderPass() };
        LotCamera camera{};
        
        auto viewerObject = LotGameObject::createGameObject();
        KeyboardMoveCtrl cameraCtrl{};

        auto currentTime = std::chrono::high_resolution_clock::now();
        while (!lotWindow.shouldClose()) {
            glfwPollEvents();
            
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraCtrl.moveInPlaneXZ(lotWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = lotRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

            // 리사이징 중일 때 특별 처리
            if (lotWindow.isUserResizing()) {
                // 리사이징 중에는 간단한 클리어만 수행
                if (auto commandBuffer = lotRenderer.beginFrame()) {
                    lotRenderer.beginSwapChainRenderPass(commandBuffer);
                    // 복잡한 렌더링 생략, 클리어만 수행됨
                    lotRenderer.endSwapChainRenderPass(commandBuffer);
                }
                lotRenderer.endFrame();
                
                // 리사이징 중에는 더 자주 폴링
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                continue;
            }

            if (auto commandBuffer = lotRenderer.beginFrame()) {
                lotRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
                lotRenderer.endSwapChainRenderPass(commandBuffer);
                lotRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(lotDevice.device());        
    }

    std::unique_ptr<LotModel> createCubeMode(LotDevice& device, glm::vec3 offset) {
        LotModel::Builder modelBuilder {};
        modelBuilder.vertices = {

            // left face (white)
            {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
            {{-.5f,  .5f,  .5f}, {.9f, .9f, .9f}},
            {{-.5f, -.5f,  .5f}, {.9f, .9f, .9f}},
            {{-.5f,  .5f, -.5f}, {.9f, .9f, .9f}},

            // right face (yellow)
            {{ .5f, -.5f, -.5f}, {.8f, .8f, .1f}},
            {{ .5f,  .5f,  .5f}, {.8f, .8f, .1f}},
            {{ .5f, -.5f,  .5f}, {.8f, .8f, .1f}},
            {{ .5f,  .5f, -.5f}, {.8f, .8f, .1f}},

            // top face (orange, remember y axis points down)
            {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
            {{ .5f, -.5f,  .5f}, {.9f, .6f, .1f}},
            {{-.5f, -.5f,  .5f}, {.9f, .6f, .1f}},
            {{ .5f, -.5f, -.5f}, {.9f, .6f, .1f}},

            // bottom face (red)
            {{-.5f,  .5f, -.5f}, {.8f, .1f, .1f}},
            {{ .5f,  .5f,  .5f}, {.8f, .1f, .1f}},
            {{-.5f,  .5f,  .5f}, {.8f, .1f, .1f}},
            {{ .5f,  .5f, -.5f}, {.8f, .1f, .1f}},

            // nose face (blue)
            {{-.5f, -.5f,  0.5f}, {.1f, .1f, .8f}},
            {{ .5f,  .5f,  0.5f}, {.1f, .1f, .8f}},
            {{-.5f,  .5f,  0.5f}, {.1f, .1f, .8f}},
            {{ .5f, -.5f,  0.5f}, {.1f, .1f, .8f}},

            // tail face (green)
            {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
            {{ .5f,  .5f, -0.5f}, {.1f, .8f, .1f}},
            {{-.5f,  .5f, -0.5f}, {.1f, .8f, .1f}},
            {{ .5f, -.5f, -0.5f}, {.1f, .8f, .1f}},

        };
        for (auto& v : modelBuilder.vertices) {
            v.position += offset;
        }

        modelBuilder.indices = {0,  1,  2,  
                                0,  3,  1,  
                                4,  5,  6,  
                                4,  7,  5,  
                                8,  9,  10, 
                                8,  11, 9,  
                                12, 13, 14, 
                                12, 15, 13, 
                                16, 17, 18, 
                                16, 19, 17, 
                                20, 21, 22, 
                                20, 23, 21};

        return std::make_unique<LotModel>(device, modelBuilder);
    }

    void FirstApp::loadGameObjects() {
        std::shared_ptr<LotModel> lotModel = createCubeMode(lotDevice, {.0f, .0f, .0f});

        auto cube = LotGameObject::createGameObject();
        cube.model = lotModel;
        cube.transform.translation = { .0f, .0f, 2.5f };
        cube.transform.scale = {.5f, .5f, .5f};
        gameObjects.push_back(std::move(cube));
    }
} // namespce lot