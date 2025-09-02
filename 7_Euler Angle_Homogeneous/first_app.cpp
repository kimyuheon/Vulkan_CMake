#include "first_app.h"

#include "simple_render_system.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace lot {
    struct SimplePushConstantData {
        glm::mat2 transform{1.f};
        glm::vec2 offset;
        alignas(16) glm::vec3 color;
    };

    FirstApp::FirstApp() { loadGameObjects(); }

    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        SimpleRenderSystem simpleRenderSystem{ lotDevice, lotRenderer.getSwapChainRenderPass() };

        while (!lotWindow.shouldClose()) {
            glfwPollEvents();
            
            if (auto commandBuffer = lotRenderer.beginFrame()) {
                lotRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
                lotRenderer.endSwapChainRenderPass(commandBuffer);
                lotRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(lotDevice.device());        
    }

    std::unique_ptr<LotModel> createCubeMode(LotDevice& device, glm::vec3 offset) {
        std::vector<LotModel::Vertex> vertices {

            // left face (white)
            {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
            {{-.5f,  .5f,  .5f}, {.9f, .9f, .9f}},
            {{-.5f, -.5f,  .5f}, {.9f, .9f, .9f}},
            {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
            {{-.5f,  .5f, -.5f}, {.9f, .9f, .9f}},
            {{-.5f,  .5f,  .5f}, {.9f, .9f, .9f}},

            // right face (yellow)
            {{ .5f, -.5f, -.5f}, {.8f, .8f, .1f}},
            {{ .5f,  .5f,  .5f}, {.8f, .8f, .1f}},
            {{ .5f, -.5f,  .5f}, {.8f, .8f, .1f}},
            {{ .5f, -.5f, -.5f}, {.8f, .8f, .1f}},
            {{ .5f,  .5f, -.5f}, {.8f, .8f, .1f}},
            {{ .5f,  .5f,  .5f}, {.8f, .8f, .1f}},

            // top face (orange, remember y axis points down)
            {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
            {{ .5f, -.5f,  .5f}, {.9f, .6f, .1f}},
            {{-.5f, -.5f,  .5f}, {.9f, .6f, .1f}},
            {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
            {{ .5f, -.5f, -.5f}, {.9f, .6f, .1f}},
            {{ .5f, -.5f,  .5f}, {.9f, .6f, .1f}},

            // bottom face (red)
            {{-.5f,  .5f, -.5f}, {.8f, .1f, .1f}},
            {{ .5f,  .5f,  .5f}, {.8f, .1f, .1f}},
            {{-.5f,  .5f,  .5f}, {.8f, .1f, .1f}},
            {{-.5f,  .5f, -.5f}, {.8f, .1f, .1f}},
            {{ .5f,  .5f, -.5f}, {.8f, .1f, .1f}},
            {{ .5f,  .5f,  .5f}, {.8f, .1f, .1f}},

            // nose face (blue)
            {{-.5f, -.5f,  0.5f}, {.1f, .1f, .8f}},
            {{ .5f,  .5f,  0.5f}, {.1f, .1f, .8f}},
            {{-.5f,  .5f,  0.5f}, {.1f, .1f, .8f}},
            {{-.5f, -.5f,  0.5f}, {.1f, .1f, .8f}},
            {{ .5f, -.5f,  0.5f}, {.1f, .1f, .8f}},
            {{ .5f,  .5f,  0.5f}, {.1f, .1f, .8f}},

            // tail face (green)
            {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
            {{ .5f,  .5f, -0.5f}, {.1f, .8f, .1f}},
            {{-.5f,  .5f, -0.5f}, {.1f, .8f, .1f}},
            {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
            {{ .5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
            {{ .5f,  .5f, -0.5f}, {.1f, .8f, .1f}},

        };
        for (auto& v : vertices) {
            v.position += offset;
        }

        return std::make_unique<LotModel>(device, vertices);
    }

    void FirstApp::loadGameObjects() {
        std::shared_ptr<LotModel> lotModel = createCubeMode(lotDevice, {.0f, .0f, .0f});

        auto cube = LotGameObject::createGameObject();
        cube.model = lotModel;
        cube.transform.translation = { .0f, .0f, .5f };
        cube.transform.scale = {.5f, .5f, .5f};
        gameObjects.push_back(std::move(cube));
    }
} // namespce lot