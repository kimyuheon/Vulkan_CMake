#include "first_app.h"

#include "keyboard_move_ctrl.h"
#include "lot_camera.h"
#include "simple_render_system.h"
#include "point_light_system.h"
#include "lot_buffer.h"
#include "lot_descriptors.h"
#include "lot_frame_info.h"
#include "sketch_manager.h"

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
#include <string>

#include <thread>

#define MAX_LIGHTS 10

namespace lot {
    FirstApp::FirstApp() { 
        loadGameObjects(); 

        sharedCubeModel = createCubeMode(lotDevice, {0.0f, 0.0f, 0.0f});
        sketchmanager.setCubeModel(sharedCubeModel);

        // DescriptorSetLayout
        globalSetLayout = LotDescriptorSetLayout::Builder(lotDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

        // 동적 크기로 변경
        size_t frameCount = lotRenderer.getFrameCount();

        // Uniform Buffer
        uboBuffers.resize(frameCount);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<LotBuffer>(
                lotDevice, sizeof(GlobalUbo), 1, 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffers[i]->map();
        }

        // Descriptor Pool
        globalPool = LotDescriptorPool::Builder(lotDevice)
        .setMaxSets(static_cast<uint32_t>(frameCount))
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(frameCount))
        .build();

        // Descriptor Sets
        globalDescriptorSets.resize(frameCount);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            LotDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        // SimpleRenderSystem
        simpleRenderSystem = std::make_unique<SimpleRenderSystem>(
            lotDevice,
            lotRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        );

        // PointLightSystem
        pointLightSystem = std::make_unique<PointLightSystem>(
            lotDevice,
            lotRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        );
    }

    FirstApp::~FirstApp() {
        //vkDeviceWaitIdle(lotDevice.device());
    }

    void FirstApp::run() {
        LotGameObject viewerObject = LotGameObject::createGameObject();
        viewerObject.transform.translation.z = -2.5f; // 카메라 초기 위치 설정

        KeyboardMoveCtrl cameraCtrl{};
        LotCamera camera{};
        
        auto projectionType = KeyboardMoveCtrl::ProjectionType::Perspective;

        // 마우스 휠 콜백 설정
        KeyboardMoveCtrl::setInstance(&cameraCtrl);
        glfwSetScrollCallback(lotWindow.getGLFWwindow(), KeyboardMoveCtrl::scrollCallback);
        glfwSetCursorPosCallback(lotWindow.getGLFWwindow(), KeyboardMoveCtrl::mouseCallback);
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        
        // 조명 토글 상태 추가
        bool enableLighting = true;
        bool gKeyPressed = false;
        glm::vec3 orbitTarget{0.0f, 0.0f, 0.0f};

        // CAD 모드로 시작
        camera.setCadMode(true);
        camera.updateCadView();
        WinTitleStr = lotWindow.getWindowTitle();
        
        if (camera.getCadMode()) {
            std::string titleStr = WinTitleStr + " [CAD]";
            if (projectionType == KeyboardMoveCtrl::ProjectionType::Perspective)
                    titleStr += "[Perspective]";
                else
                    titleStr += "[Orthographic]";
            glfwSetWindowTitle(lotWindow.getGLFWwindow(), titleStr.c_str());
        } else {
            std::string titleStr = WinTitleStr + " [View]";
            if (projectionType == KeyboardMoveCtrl::ProjectionType::Perspective)
                    titleStr += "[Perspective]";
                else
                    titleStr += "[Orthographic]";
            glfwSetWindowTitle(lotWindow.getGLFWwindow(), titleStr.c_str());
        }

        while (!lotWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            float aspect = lotRenderer.getAspectRatio();
            
            // 입력 처리(키보드 + CAD 마우스)
            handleInputs(newTime, viewerObject, camera, cameraCtrl, enableLighting, 
                         gKeyPressed, projectionType, frameTime, orbitTarget);
            
            // FPS 모드일 시 WASD + 마우스
            if (!camera.getCadMode()) {
                updateCamera(cameraCtrl, frameTime, viewerObject, orbitTarget, projectionType);
            }

            updateProjection(camera, projectionType, aspect, viewerObject, orbitTarget);

            // 렌더링
            if (lotWindow.isUserResizing()) {
                handleResizing();
                continue;
            }
            
            if (auto commandBuffer = lotRenderer.beginFrame()) {
                int frameIndex = lotRenderer.getFrameIndex();

                FrameInfo frameInfo {
                    frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects
                };

                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.View = camera.getView();
                ubo.inverseView = camera.getInverseView();
                ubo.ambientLightColor = glm::vec4(1.f, 1.f, 1.f, .02f);
                
                // === 게임 로직 업데이트 ===
                pointLightSystem->update(frameInfo, ubo);
                
                ubo.lightingEnabled = enableLighting ? 1 : 0;
                
                // === GPU에 UBO 전송 ===
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();
                
                lotRenderer.beginSwapChainRenderPass(commandBuffer);
                
                simpleRenderSystem->renderGameObjects(frameInfo);

                if (sketchmanager.isSketchActive())
                    renderSketchPreview(frameInfo);

                pointLightSystem->render(frameInfo);
                simpleRenderSystem->renderHighlights(frameInfo);
                lotRenderer.endSwapChainRenderPass(commandBuffer);

                lotRenderer.endFrame();
            }
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

    void FirstApp::handleInputs(const std::chrono::high_resolution_clock::time_point& currentTime, 
                                LotGameObject& viewerObject, LotCamera& camera, KeyboardMoveCtrl& cameraCtrl, 
                                bool& enableLighting, 
                                bool& gKeyPressed, KeyboardMoveCtrl::ProjectionType& projectionType, 
                                float frameTime, glm::vec3& orbitTarget) {
        auto* window = lotWindow.getGLFWwindow();

        // 객체 선택 처리 (메인 카메라 사용)
        if (!sketchmanager.isSketchActive()) {
            selectionManager.handleMouseClick(window, camera, gameObjects);
        } else {
            // 스케치 모드 중 마우스 상태 동기화 (스케치 완료 시 클릭이 선택으로 처리되지 않도록)
            selectionManager.syncMouseState(window);
        }

        // B키: 스케치 모드 시작
        static bool bKeyPressed = false;
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bKeyPressed) {
            bKeyPressed = true;
            if (!sketchmanager.isSketchActive()) {
                selectionManager.clearAllSelections(gameObjects);
                sketchmanager.startSketch();
            }
        }
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE) {
            bKeyPressed = false;
        } 

        // 스케치 매니저 업데이트p
        sketchmanager.handleInput(window, camera, gameObjects);

        // 키보드 입력 처리
        static bool keyPressed = false;

        // C키: 카메라 모드 전환
        static bool ckeyPressed = false;
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !ckeyPressed) {
            ckeyPressed = true;
            camera.setCadMode(!camera.getCadMode());
            if (camera.getCadMode()) {
                std::string titleStr = WinTitleStr + " [CAD]";
                if (projectionType == KeyboardMoveCtrl::ProjectionType::Perspective)
                    titleStr += "[Perspective]";
                else
                    titleStr += "[Orthographic]";
                glfwSetWindowTitle(window, titleStr.c_str());
            } else {
                std::string titleStr = WinTitleStr + " [View]";
                if (projectionType == KeyboardMoveCtrl::ProjectionType::Perspective)
                    titleStr += "[Perspective]";
                else
                    titleStr += "[Orthographic]";
                glfwSetWindowTitle(window, titleStr.c_str());
            }

            std::cout << "Cameara mode: " << (camera.getCadMode() ? "CAD" : "FPS") << std::endl;
        }
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
            ckeyPressed = false;
        }

        // O키: 카메라 모드 전환
        static bool okeyPressed = false;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !okeyPressed) {
            okeyPressed = true;
            std::string titleStr;
            if (camera.getCadMode())
                titleStr = WinTitleStr + " [CAD]";
            else 
                titleStr = WinTitleStr + " [View]";
            if (projectionType == KeyboardMoveCtrl::ProjectionType::Perspective) {
                projectionType = KeyboardMoveCtrl::ProjectionType::Orthographic;
                std::cout << "Projection: Orthographics" << std::endl;
                titleStr += "[Orthographic]";
            } else {
                projectionType = KeyboardMoveCtrl::ProjectionType::Perspective;
                std::cout << "Projection: Perspective" << std::endl;
                titleStr += "[Perspective]";
            }
            glfwSetWindowTitle(window, titleStr.c_str());
        }
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE) {
            okeyPressed = false;
        }

        // R키: 카메라 리셋
        // 숫자키: CAD 뷰 방향 변경
        static bool viewKeyPressed = false;
        if (camera.getCadMode()) {
            LotCamera::CadViewType viewType;
            bool hasInput = false;
            
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
                viewType = LotCamera::CadViewType::Front;
                hasInput = true;
            } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
                viewType = LotCamera::CadViewType::Top;
                hasInput = true;
            } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
                viewType = LotCamera::CadViewType::Right;
                hasInput = true;
            } else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
                viewType = LotCamera::CadViewType::Isometric;
                hasInput = true;
            }
            
            if (hasInput && !viewKeyPressed) {
                viewKeyPressed = true;
                camera.resetCadRotation(viewType);
                std::cout << "CAD View changed!" << std::endl;
            }
            
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE &&
                glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE &&
                glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE &&
                glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE) {
                viewKeyPressed = false;
            }
        }

        // ESC: 모든 선택 해제
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            selectionManager.clearAllSelections(gameObjects);
        }

        // G: 조명 토글
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !gKeyPressed) {
            gKeyPressed = true;
            enableLighting = !enableLighting;
            std::cout << "Lighting " << (enableLighting ? "ENABLED" : "DISABLED") << std::endl;
        }

        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) {
            gKeyPressed = false;
        }

        // N: 새 큐브 추가
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !keyPressed) {
            keyPressed = true;
            addNewCube();
            std::cout << "New cube added! Total objects: " << gameObjects.size() << std::endl;
        }

        // Delete: 선택된 객체 삭제
        if (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS && !keyPressed) {
            keyPressed = true;
            removeSelectedObjects();
            std::cout << "Selected objects removed! Total objects: " << gameObjects.size() << std::endl;
        }

        // 키 릴리스 체크
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE &&
            glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_RELEASE) {
            keyPressed = false;
        }

        if (camera.getCadMode()) {
            static double lastMouseX = 0.0f;
            static double lastMouseY = 0.0f;
            static bool firstMouse = true;
            static bool isOrbitingActive = false;
            static bool isPanningActive = false;

            double currentMouseX, currentMouseY;
            glfwGetCursorPos(window, &currentMouseX, &currentMouseY);

            if (firstMouse) {
                lastMouseX = currentMouseX;
                lastMouseY = currentMouseY;
                firstMouse = false;
            }

            float deltaX = static_cast<float>(currentMouseX - lastMouseX);
            float deltaY = static_cast<float>(currentMouseY - lastMouseY);

            // 우클릭 드래그: Orbit(회전)
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                if (!isOrbitingActive) {
                    isOrbitingActive = true;
                } else {
                    float rotationSpeed = 0.01f;
                    camera.orbitAroundTarget(deltaX * rotationSpeed, -deltaY * rotationSpeed);
                }
            } else {
                isOrbitingActive = false;
            }

            // 중간클릭 드래그: Pan
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
                if (!isPanningActive) {
                    isPanningActive = true;
                } else {
                    int width, height;
                    glfwGetWindowSize(window, &width, &height);
                    float aspect = static_cast<float>(width) / static_cast<float>(height);

                    // 픽셀을 NDC 단위로 변환 (-1 ~ 1)
                    float ndcDeltaX = (2.0f * deltaX) / width;
                    float ndcDeltaY = (2.0f * deltaY) / height;

                    bool isOrtho = (projectionType == KeyboardMoveCtrl::ProjectionType::Orthographic);
                    camera.panTarget(-ndcDeltaX, ndcDeltaY, isOrtho, aspect);
                }
            } else {
                isPanningActive = false;
            }

            // 마우스 휠: Zoom
            double scroll = cameraCtrl.getScrollDelta();
            if (scroll != 0.0) {
                //std::cout << "Mouse Wheel Working!!!" << std::endl;
                float zoomSpeed = 1.0f;
                bool isOrtho = (projectionType == KeyboardMoveCtrl::ProjectionType::Orthographic);

                // 마우스 위치의 월드 좌표 계산
                int width, height;
                glfwGetWindowSize(window, &width, &height);

                glm::mat4 invProj = glm::inverse(camera.getProjection());
                glm::mat4 invView = glm::inverse(camera.getView());

                float ndcX = static_cast<float>((2.0f * currentMouseX) / width - 1.0f);
                float ndcY = static_cast<float>((2.0f * currentMouseY) / height - 1.0f);

                glm::vec4 clipNear = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
                glm::vec4 viewNear = invProj * clipNear;
                viewNear /= viewNear.w;
                glm::vec3 worldNear = glm::vec3(invView * viewNear);

                camera.zoomToTarget(static_cast<float>(scroll) * zoomSpeed, isOrtho, &worldNear);
                cameraCtrl.resetScrollDelta();
            }

            lastMouseX = currentMouseX;
            lastMouseY = currentMouseY;
        } else {
            // 스크롤 입력
            cameraCtrl.processScrollInput(window, projectionType, 
                                          orthoSize, viewerObject, orbitTarget,
                                          glm::radians(50.f));

            // WASD 이동
            cameraCtrl.moveInPlaneXZ(window, frameTime, viewerObject);

            // 마우스 카메라 제어
            cameraCtrl.handleMouseCameraControlWithProjection(window, frameTime, viewerObject, orbitTarget,
                                                              orthoSize, lotRenderer.getAspectRatio());
        }

        // 디버그 출력 (5초마다)
        printDebugInfo(currentTime, viewerObject, enableLighting);
    }

    void FirstApp::updateProjection(LotCamera& camera, KeyboardMoveCtrl::ProjectionType projectionType, float aspect, const LotGameObject& viewerObject, const glm::vec3& orbitTarget) {

        // 3DS Max 스타일 자유 회전 - 무제한 상하좌우 회전
        if (!camera.getCadMode()) {
            camera.setViewFromTransform(viewerObject.transform.translation, viewerObject.transform.rotation);
        } 

        // 투영 설정 (기존 유지)
        if (projectionType == KeyboardMoveCtrl::ProjectionType::Perspective) {
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.01f, 100.f);
        } else {
            float orthoSize = camera.getOrthoSize();
            camera.setOrthographicProjection(
                -orthoSize * aspect, orthoSize * aspect,
                -orthoSize, orthoSize,
                0.01f, 100.f
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
            for (const auto& kv : gameObjects) {
                auto& obj = kv.second;
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

    void FirstApp::renderSketchPreview(FrameInfo& frameInfo) {
        auto& previewMap = sketchmanager.getPreviewMap();
        if (previewMap.empty()) return;

        FrameInfo previewFrameInfo{
            frameInfo.frameIndex,
            frameInfo.frameTime,
            frameInfo.commandBuffer,
            frameInfo.camera,
            frameInfo.globalDescriptorSet,
            previewMap
        };

        simpleRenderSystem->renderGameObjects(previewFrameInfo);
    }

    std::unique_ptr<LotModel> createCubeMode(LotDevice& device, glm::vec3 offset) {
        LotModel::Builder modelBuilder {};
        modelBuilder.vertices = {

            // left face (white) - normal pointing left (-X)
            {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}, {-1.f, 0.f, 0.f}},
            {{-.5f,  .5f,  .5f}, {.9f, .9f, .9f}, {-1.f, 0.f, 0.f}},
            {{-.5f, -.5f,  .5f}, {.9f, .9f, .9f}, {-1.f, 0.f, 0.f}},
            {{-.5f,  .5f, -.5f}, {.9f, .9f, .9f}, {-1.f, 0.f, 0.f}},

            // right face (yellow) - normal pointing right (+X)
            {{ .5f, -.5f, -.5f}, {.8f, .8f, .1f}, {1.f, 0.f, 0.f}},
            {{ .5f,  .5f,  .5f}, {.8f, .8f, .1f}, {1.f, 0.f, 0.f}},
            {{ .5f, -.5f,  .5f}, {.8f, .8f, .1f}, {1.f, 0.f, 0.f}},
            {{ .5f,  .5f, -.5f}, {.8f, .8f, .1f}, {1.f, 0.f, 0.f}},

            // top face (orange, remember y axis points down) - normal pointing up (-Y)
            {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}, {0.f, -1.f, 0.f}},
            {{ .5f, -.5f,  .5f}, {.9f, .6f, .1f}, {0.f, -1.f, 0.f}},
            {{-.5f, -.5f,  .5f}, {.9f, .6f, .1f}, {0.f, -1.f, 0.f}},
            {{ .5f, -.5f, -.5f}, {.9f, .6f, .1f}, {0.f, -1.f, 0.f}},

            // bottom face (red) - normal pointing down (+Y)
            {{-.5f,  .5f, -.5f}, {.8f, .1f, .1f}, {0.f, 1.f, 0.f}},
            {{ .5f,  .5f,  .5f}, {.8f, .1f, .1f}, {0.f, 1.f, 0.f}},
            {{-.5f,  .5f,  .5f}, {.8f, .1f, .1f}, {0.f, 1.f, 0.f}},
            {{ .5f,  .5f, -.5f}, {.8f, .1f, .1f}, {0.f, 1.f, 0.f}},

            // nose face (blue) - normal pointing forward (+Z)
            {{-.5f, -.5f,  0.5f}, {.1f, .1f, .8f}, {0.f, 0.f, 1.f}},
            {{ .5f,  .5f,  0.5f}, {.1f, .1f, .8f}, {0.f, 0.f, 1.f}},
            {{-.5f,  .5f,  0.5f}, {.1f, .1f, .8f}, {0.f, 0.f, 1.f}},
            {{ .5f, -.5f,  0.5f}, {.1f, .1f, .8f}, {0.f, 0.f, 1.f}},

            // tail face (green) - normal pointing backward (-Z)
            {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}, {0.f, 0.f, -1.f}},
            {{ .5f,  .5f, -0.5f}, {.1f, .8f, .1f}, {0.f, 0.f, -1.f}},
            {{-.5f,  .5f, -0.5f}, {.1f, .8f, .1f}, {0.f, 0.f, -1.f}},
            {{ .5f, -.5f, -0.5f}, {.1f, .8f, .1f}, {0.f, 0.f, -1.f}},

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
        cube1.color = {0.0f, 0.0f, 0.0f};  // 버텍스 색상 사용
        gameObjects.emplace(cube1.getId(), std::move(cube1));

        auto cube2 = LotGameObject::createGameObject();
        cube2.model = lotModel;
        cube2.transform.translation = { 1.5f, .0f, 2.5f };
        cube2.transform.scale = {.3f, .3f, .3f};
        cube2.color = {0.0f, 0.0f, 0.0f};  // 버텍스 색상 사용
        gameObjects.emplace(cube2.getId(), std::move(cube2));

        auto cube3 = LotGameObject::createGameObject();
        cube3.model = lotModel;
        cube3.transform.translation = { -1.5f, .0f, 2.5f };
        cube3.transform.scale = {.4f, .4f, .4f};
        cube3.color = {0.0f, 0.0f, 0.0f};  // 버텍스 색상 사용
        gameObjects.emplace(cube3.getId(), std::move(cube3));

        std::shared_ptr<LotModel> objModel = 
            LotModel::createModelFromFile(lotDevice, "models/smooth_vase.obj");
        auto obj = LotGameObject::createGameObject();
        obj.model = objModel;
        obj.transform.translation = { .0f, .5f, -.5f };
        obj.transform.scale = glm::vec3(3.f);
        obj.color = {0.3f, 0.5f, 0.8f};
        gameObjects.emplace(obj.getId(), std::move(obj));

        objModel = LotModel::createModelFromFile(lotDevice, "models/flat_vase.obj");
        auto flat_vase = LotGameObject::createGameObject();
        flat_vase.model = objModel;
        flat_vase.transform.translation = { .0f, .0f, 1.f };
        flat_vase.transform.scale = glm::vec3{ 2.5f, 1.5f, 2.5f };
        flat_vase.color = {0.3f, 0.5f, 0.8f};
        gameObjects.emplace(flat_vase.getId(), std::move(flat_vase));

        objModel = LotModel::createModelFromFile(lotDevice, "models/quad.obj");
        auto quad_vase = LotGameObject::createGameObject();
        quad_vase.model = objModel;
        quad_vase.transform.translation = { 0.f, .5f, 2.5f };
        quad_vase.transform.scale = glm::vec3{ 4.f, 1.f, 4.f };
        gameObjects.emplace(quad_vase.getId(), std::move(quad_vase));

        std::vector<glm::vec3> lightColors{
            {  1.f,  .1f,  .1f},
            {  .1f,  .1f,  1.f},
            {  .1f,  1.f,  .1f},
            {  1.f,  1.f,  .1f},
            {  .1f,  1.f,  1.f},
            {  1.f,  1.f,  1.f}
        };

        for (int i = 0; i < lightColors.size(); i++) {
            auto pointLight = LotGameObject::makePointLight(0.2f);
            pointLight.color = lightColors[i];
            auto rotateLight = glm::rotate(glm::mat4(1.f), 
                                          (i * glm::two_pi<float>() / lightColors.size()),
                                          {0.f, -1.f, 0.f});
            pointLight.transform.translation = 
                glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }
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

        gameObjects.emplace(newCube.getId(), std::move(newCube));
    }

    void FirstApp::removeSelectedObjects() {
        /* gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                [](const LotGameObject& obj) { return obj.isSelected; }),
            gameObjects.end()
        ); */

        for (auto it = gameObjects.begin(); it != gameObjects.end();) {
            if (it->second.isSelected) {
                it = gameObjects.erase(it);
            } else {
                ++it;
            }
        }

        selectionManager.clearAllSelections(gameObjects);
    }
} // namespce lot