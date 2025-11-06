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
#include <algorithm>
#include <cstdlib>
#include <iostream>

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
        viewerObject.transform.translation = {0.0f, 0.0f, 0.0f}; // 카메라 초기 위치 설정
        KeyboardMoveCtrl cameraCtrl{};
        glm::vec3 orbitTarget{0.0f, 0.0f, 2.5f};

        // 마우스 휠 콜백 설정
        auto projectionType = KeyboardMoveCtrl::ProjectionType::Perspective;
        KeyboardMoveCtrl::setInstance(&cameraCtrl);
        glfwSetScrollCallback(lotWindow.getGLFWwindow(), KeyboardMoveCtrl::scrollCallback);
        glfwSetCursorPosCallback(lotWindow.getGLFWwindow(), KeyboardMoveCtrl::mouseCallback);

        // 조명 토글 상태 추가
        bool enableLighting = true;
        bool gKeyPressed = false;

        auto currentTime = std::chrono::high_resolution_clock::now();
        while (!lotWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            float aspect = lotRenderer.getAspectRatio();

            // 입력 처리 및 업데이트
            updateCamera(cameraCtrl, frameTime, viewerObject, orbitTarget, projectionType);
            updateProjection(camera, projectionType, aspect, viewerObject, orbitTarget);
            handleInputs(newTime, viewerObject, camera, enableLighting, gKeyPressed);

            // 렌더링
            if (lotWindow.isUserResizing()) {
                handleResizing();
                continue;
            }
            //std::cout << "render Lighting" << (enableLighting ? "Light On" : "Light Off") << std::endl;
            render(simpleRenderSystem, camera, enableLighting);
        }
        vkDeviceWaitIdle(lotDevice.device());
    }

    void FirstApp::updateCamera(KeyboardMoveCtrl& cameraCtrl, float frameTime,
                               LotGameObject& viewerObject, glm::vec3& orbitTarget,
                               KeyboardMoveCtrl::ProjectionType projectionType) {
        // 카메라 이동 제어
        cameraCtrl.moveInPlaneXZ(lotWindow.getGLFWwindow(), frameTime, viewerObject);

        // 객체 회전 처리
        cameraCtrl.rotateObjects(lotWindow.getGLFWwindow(), frameTime, gameObjects);

        // 투영 관련 설정
        float orthoSize = 1.0f;
        float fov = glm::radians(50.0f);
        float aspect = lotRenderer.getAspectRatio();

        // 마우스 줄 처리
        cameraCtrl.processScrollInput(lotWindow.getGLFWwindow(), projectionType,
                                     orthoSize, viewerObject, orbitTarget, fov);

        // 마우스 카메라 제어
        cameraCtrl.handleMouseCameraControlWithProjection(
            lotWindow.getGLFWwindow(), frameTime, viewerObject, orbitTarget,
            orthoSize, aspect
        );
    }

    void FirstApp::handleInputs(const std::chrono::high_resolution_clock::time_point& currentTime, const LotGameObject& viewerObject, LotCamera& camera,
                                bool& enableLighting, bool& gKeyPressed) {
        // 객체 선택 처리 (메인 카메라 사용)
        selectionManager.handleMouseClick(lotWindow.getGLFWwindow(), camera, gameObjects);

        // 키보드 입력 처리
        static bool keyPressed = false;

        // ESC: 모든 선택 해제
        if (glfwGetKey(lotWindow.getGLFWwindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            selectionManager.clearAllSelections(gameObjects);
        }

        // G: 조명 토글
        if (glfwGetKey(lotWindow.getGLFWwindow(), GLFW_KEY_G) == GLFW_PRESS && !gKeyPressed) {
            gKeyPressed = true;
            enableLighting = !enableLighting;
            std::cout << "Lighting " << (enableLighting ? "ENABLED" : "DISABLED") << std::endl;
        }

        if (glfwGetKey(lotWindow.getGLFWwindow(), GLFW_KEY_G) == GLFW_RELEASE) {
            gKeyPressed = false;
        }

        // N: 새 큐브 추가
        if (glfwGetKey(lotWindow.getGLFWwindow(), GLFW_KEY_N) == GLFW_PRESS && !keyPressed) {
            keyPressed = true;
            addNewCube();
            std::cout << "New cube added! Total objects: " << gameObjects.size() << std::endl;
        }

        // Delete: 선택된 객체 삭제
        if (glfwGetKey(lotWindow.getGLFWwindow(), GLFW_KEY_DELETE) == GLFW_PRESS && !keyPressed) {
            keyPressed = true;
            removeSelectedObjects();
            std::cout << "Selected objects removed! Total objects: " << gameObjects.size() << std::endl;
        }

        // 키 릴리스 체크
        if (glfwGetKey(lotWindow.getGLFWwindow(), GLFW_KEY_N) == GLFW_RELEASE &&
            glfwGetKey(lotWindow.getGLFWwindow(), GLFW_KEY_DELETE) == GLFW_RELEASE) {
            keyPressed = false;
        }

        // 디버그 출력 (5초마다)
        printDebugInfo(currentTime, viewerObject, enableLighting);
    }

    void FirstApp::updateProjection(LotCamera& camera, KeyboardMoveCtrl::ProjectionType projectionType, float aspect, const LotGameObject& viewerObject, const glm::vec3& orbitTarget) {

        // 3DS Max 스타일 자유 회전 - 무제한 상하좌우 회전
        camera.setViewFromTransform(viewerObject.transform.translation, viewerObject.transform.rotation);
        
        // 투영 설정 (기존 유지)
        if (projectionType == KeyboardMoveCtrl::ProjectionType::Perspective) {
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);
        } else {
            float orthoSize = 1.0f;
            camera.setOrthographicProjection(
                -orthoSize * aspect, orthoSize * aspect,
                -orthoSize, orthoSize,
                0.1f, 10.f
            );
        }
    }

    void FirstApp::handleResizing() {
        // 리사이징 중에는 간단한 클리어만 수행
        if (auto commandBuffer = lotRenderer.beginFrame()) {
            lotRenderer.beginSwapChainRenderPass(commandBuffer);
            lotRenderer.endSwapChainRenderPass(commandBuffer);
        }
        lotRenderer.endFrame();

        // 리사이징 중에는 더 자주 폴링
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    void FirstApp::render(SimpleRenderSystem& renderSystem, LotCamera& camera, bool enableLighting) {
        if (auto commandBuffer = lotRenderer.beginFrame()) {
            lotRenderer.beginSwapChainRenderPass(commandBuffer);
            renderSystem.renderGameObjects(commandBuffer, gameObjects, camera, enableLighting);
            renderSystem.renderHighlights(commandBuffer, gameObjects, camera, enableLighting);
            lotRenderer.endSwapChainRenderPass(commandBuffer);
            lotRenderer.endFrame();
        }
    }

    void FirstApp::printDebugInfo(const std::chrono::high_resolution_clock::time_point& currentTime,
                                 const LotGameObject& viewerObject, bool enableLighting) {
        static auto lastInfoTime = std::chrono::high_resolution_clock::now();
        static auto lastCameraDebug = std::chrono::high_resolution_clock::now();

        // 씬 정보 출력 (5초마다)
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastInfoTime).count() >= 5) {
            std::cout << "=== Scene Info ===" << std::endl;
            std::cout << "Total objects: " << gameObjects.size() << std::endl;
            std::cout << "Lighting: " << (enableLighting ? "ON" : "OFF") << std::endl;

            int selectedCount = 0;
            for (const auto& obj : gameObjects) {
                if (obj.isSelected) selectedCount++;
                std::cout << "Object ID " << obj.getId()
                          << " - Pos: (" << obj.transform.translation.x << ", "
                          << obj.transform.translation.y << ", " << obj.transform.translation.z
                          << "), Scale: (" << obj.transform.scale.x << ", "
                          << obj.transform.scale.y << ", " << obj.transform.scale.z
                          << "), Selected: " << (obj.isSelected ? "Yes" : "No") << std::endl;
            }

            std::cout << "Selected objects: " << selectedCount << "/" << gameObjects.size() << std::endl;
            std::cout << "==================" << std::endl;
            lastInfoTime = currentTime;
        }

        // 카메라 디버그 정보 출력 (5초마다)
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastCameraDebug).count() >= 5) {
            std::cout << "Camera Position: (" << viewerObject.transform.translation.x << ", "
                      << viewerObject.transform.translation.y << ", " << viewerObject.transform.translation.z << ")" << std::endl;
            std::cout << "Camera Rotation (Quat): w=" << viewerObject.transform.rotation.w 
                      << " x=" << viewerObject.transform.rotation.x 
                      << " y=" << viewerObject.transform.rotation.y 
                      << " z=" << viewerObject.transform.rotation.z << std::endl;
            lastCameraDebug = currentTime;
        }
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

        auto cube1 = LotGameObject::createGameObject();
        cube1.model = lotModel;
        cube1.transform.translation = { .0f, .0f, 2.5f };
        cube1.transform.scale = {.5f, .5f, .5f};
        cube1.color = {1.0f, 0.0f, 0.0f};
        gameObjects.push_back(std::move(cube1));

        auto cube2 = LotGameObject::createGameObject();
        cube2.model = lotModel;
        cube2.transform.translation = { 1.5f, .0f, 2.5f };
        cube2.transform.scale = {.3f, .3f, .3f};
        cube2.color = {0.0f, 1.0f, 0.0f};
        gameObjects.push_back(std::move(cube2));

        auto cube3 = LotGameObject::createGameObject();
        cube3.model = lotModel;
        cube3.transform.translation = { -1.5f, .0f, 2.5f };
        cube3.transform.scale = {.4f, .4f, .4f};
        cube3.color = {0.0f, 0.0f, 1.0f};
        gameObjects.push_back(std::move(cube3));

        std::shared_ptr<LotModel> objModel = 
            LotModel::createModelFromFile(lotDevice, "models/smooth_vase.obj");
        auto obj = LotGameObject::createGameObject();
        obj.model = objModel;
        obj.transform.translation = { .0f, .0f, 1.5f };
        obj.transform.scale = glm::vec3(3.f);
        gameObjects.push_back(std::move(obj));
    }

    void FirstApp::addNewCube() {
        std::shared_ptr<LotModel> lotModel = createCubeMode(lotDevice, {.0f, .0f, .0f});

        auto newCube = LotGameObject::createGameObject();
        newCube.model = lotModel;

        // 랜덤 위치와 스케일
        float randomX = ((rand() % 200) - 100) / 50.0f;  // -2.0 to 2.0
        float randomY = ((rand() % 100) - 50) / 50.0f;   // -1.0 to 1.0
        float randomScale = ((rand() % 50) + 20) / 100.0f;  // 0.2 to 0.7

        newCube.transform.translation = {randomX, randomY, 2.5f};
        newCube.transform.scale = {randomScale, randomScale, randomScale};

        // 랜덤 색상
        newCube.color = {
            (rand() % 100) / 100.0f,
            (rand() % 100) / 100.0f,
            (rand() % 100) / 100.0f
        };

        gameObjects.push_back(std::move(newCube));
    }

    void FirstApp::removeSelectedObjects() {
        gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                [](const LotGameObject& obj) { return obj.isSelected; }),
            gameObjects.end()
        );

        selectionManager.clearAllSelections(gameObjects);
    }
} // namespce lot