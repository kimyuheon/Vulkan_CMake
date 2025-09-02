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

    void FirstApp::loadGameObjects() {
        std::vector<LotModel::Vertex> vertices {
            {{  0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
            {{  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
            {{ -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }}
        };
        auto lotModel = std::make_shared<LotModel>(lotDevice, vertices);

        auto triangle = LotGameObject::createGameObject();
        triangle.model = lotModel;
        triangle.color = {.1f, .8f, .1f};
        triangle.transform2d.translation.x = .2f;
        triangle.transform2d.scale = {2.f, .5f};
        triangle.transform2d.rotation = .25f * glm::two_pi<float>();

        gameObjects.push_back(std::move(triangle));
    }
} // namespce lot