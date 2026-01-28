#include "sketch_manager.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace lot {
    SketchManager::SketchManager(LotDevice& device) : lotDevice{device} {}

    void SketchManager::startSketch() {
        state = SketchState::WaitingFirstClick;
        previewMap.clear();
        firstPointSet = false;
        std::cout << "=== 스케치 모드 시작 ===" << std::endl;
        std::cout << "1단계: 클릭하여 첫 점 설정" << std::endl;
    }

    void SketchManager::cancelSketch() {
        state = SketchState::Idle;
        previewMap.clear();
        firstPointSet = false;
        std::cout << "=== 스케치 모드 취소 ===" << std::endl;
    }

    void SketchManager::handleInput(GLFWwindow* window, const LotCamera& camera, LotGameObject::Map& gameObjects) {
        // ESC 스케치 취소
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !keyPressed) {
            keyPressed = true;
            if (state != SketchState::Idle) {
                cancelSketch();    
            }
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
            keyPressed = false;
        }

        // 스케치모드 아닐 시 리턴
        if (state == SketchState::Idle) return;

        // 마우스 커서 가져오기
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        currentScreenY = static_cast<float>(mouseY);  // 현재 화면 Y 저장

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        // 현재 마우스 위치를 월드 좌표로 변경
        currentMousePos = screenToWorldPlane(mouseX, mouseY, windowWidth, windowHeight, camera, baseHeight);

        // 상태에 따른 프리뷰 업데이트
        if (state == SketchState::WaitingFirstClick || state == SketchState::WaitingSecondClick)
            updateRectanglePreview();
        else if (state == SketchState::WaitingHeight)
            updateBoxPreview();

        // 마우스 클릭 처리
        int leftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

        if (leftState == GLFW_PRESS && !leftMousePressed) {
            leftMousePressed = true;

            switch (state)
            {
            case SketchState::WaitingFirstClick:
            {
                // 첫번재 클릭
                firstPoint = currentMousePos;
                std::cout << "First click at: (" << 
                    firstPoint.x << ", " << firstPoint.y << ", " << firstPoint.z << ")" << std::endl;
                baseHeight = firstPoint.y;
                firstPointSet = true;
                state = SketchState::WaitingSecondClick;
                std::cout << "2단계: 마우스 이동으로 사각형 크기 조절 후 클릭" << std::endl;
                break;
            }
            case SketchState::WaitingSecondClick:
            {
                // 두 번째 클릭 - 사각형 확정
                secondPoint = currentMousePos;
                secondClickScreenY = currentScreenY;  // 높이 조절 기준점 저장
                state = SketchState::WaitingHeight;
                std::cout << "3단계: 마우스를 위아래로 움직여 높이 조정 후 클릭하여 완성" << std::endl;

                break;
            }
            case SketchState::WaitingHeight:
            {
                // 최종 완성
                auto finalBox = createFinalBox(gameObjects);
                gameObjects.emplace(finalBox.getId(), std::move(finalBox));
                std::cout << "=== 박스 생성 완료! 총 객체 수: " << gameObjects.size() << " ===" << std::endl;

                // 스케치 완료
                state = SketchState::Idle;
                previewMap.clear();
                firstPointSet = false;

                break;
            }
            default:
                break;
            }
        }
        if (leftState == GLFW_RELEASE) {
            leftMousePressed = false;
        }
    }

    glm::vec3 SketchManager::screenToWorldPlane(double mouseX, double mouseY, int windowWidth, int windowHeight,
                                               const LotCamera& camera, float planeHeight) {
        // NDC 좌표로 변환
        float ndcX = (2.0f * static_cast<float>(mouseX)) / windowWidth - 1.0f;
        float ndcY = (2.0f * static_cast<float>(mouseY)) / windowHeight - 1.0f;

        // Near/Far 클립 좌표
        glm::vec4 clipNear = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 clipFar = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

        // 역투영-뷰 행렬
        glm::mat4 invProjView = glm::inverse(camera.getProjection() * camera.getView());

        // 월드 좌표로 변환
        glm::vec4 worldNear = invProjView * clipNear;
        worldNear /= worldNear.w;
        glm::vec4 worldFar = invProjView * clipFar;
        worldFar /= worldFar.w;

        glm::vec3 rayOrigin = glm::vec3(worldNear);
        glm::vec3 rayDir = glm::normalize(glm::vec3(worldFar - worldNear));

        glm::vec3 targetPos = camera.getTarget();

        float t = 0.0f;

        // 뷰 타입에 따라 교차 평면 결정
        switch (camera.getCurrentViewType()) {
            case LotCamera::CadViewType::Top:
            case LotCamera::CadViewType::Isometric:
                // XZ 평면 (Y = targetPos.y)
                if (std::abs(rayDir.y) < 0.0001f) return targetPos;
                t = (targetPos.y - rayOrigin.y) / rayDir.y;
                break;
            case LotCamera::CadViewType::Front:
                // XY 평면 (Z = targetPos.z)
                if (std::abs(rayDir.z) < 0.0001f) return targetPos;
                t = (targetPos.z - rayOrigin.z) / rayDir.z;
                break;
            case LotCamera::CadViewType::Right:
                // YZ 평면 (X = targetPos.x)
                if (std::abs(rayDir.x) < 0.0001f) return targetPos;
                t = (targetPos.x - rayOrigin.x) / rayDir.x;
                break;
            default:
                if (std::abs(rayDir.y) < 0.0001f) return targetPos;
                t = (planeHeight - rayOrigin.y) / rayDir.y;
                break;
        }

        glm::vec3 worldPos = rayOrigin + t * rayDir;
        return worldPos;
    }

    void SketchManager::updateRectanglePreview() {
        previewMap.clear();

        if (!firstPointSet) return;

        glm::vec3 min = glm::vec3(std::min(firstPoint.x, currentMousePos.x),
                                  baseHeight, 
                                  std::min(firstPoint.z, currentMousePos.z));

        glm::vec3 max = glm::vec3(std::max(firstPoint.x, currentMousePos.x),
                                  baseHeight, 
                                  std::max(firstPoint.z, currentMousePos.z));

        // 얇은 박스로 바닥면 프리뷰
        auto previewBox = LotGameObject::createGameObject();

        glm::vec3 center = (min + max) * 0.5f;
        glm::vec3 size = max - min;

        // 너무 적을 시 예외처리
        if (size.x < 0.1f) size.x = 0.1f;
        if (size.z < 0.1f) size.z = 0.1f;

        previewBox.transform.translation = center;
        previewBox.transform.scale = glm::vec3(size.x, 0.05f, size.z);  // 얇은 바닥면
        previewBox.color = glm::vec3(0.2f, 0.8f, 1.0f);  // 하늘색

        previewBox.model = cubeModel;
        previewMap.emplace(previewBox.getId(), std::move(previewBox));
    }

    void SketchManager::updateBoxPreview() {
        previewMap.clear();

        // 화면 Y 움직임을 높이로 변환 (위로 이동 = 양의 높이)
        float height = (currentScreenY - secondClickScreenY) * heightScale;
        if (std::abs(height) < 0.1f)
            height = (height >= 0 ? 0.1f : -0.1f);

        glm::vec3 min = glm::vec3(std::min(firstPoint.x, secondPoint.x),
                                  baseHeight,
                                  std::min(firstPoint.z, secondPoint.z));

        glm::vec3 max = glm::vec3(std::max(firstPoint.x, secondPoint.x),
                                  baseHeight + height,
                                  std::max(firstPoint.z, secondPoint.z));

        // 얇은 박스로 바닥면 프리뷰
        auto previewBox = LotGameObject::createGameObject();

        glm::vec3 center = (min + max) * 0.5f;
        glm::vec3 size = max - min;

        previewBox.transform.translation = center;
        previewBox.transform.scale = glm::vec3(
            std::abs(size.x),
            std::abs(size.y),
            std::abs(size.z)
        );
        previewBox.color = glm::vec3(0.2f, 0.8f, 1.0f);
        previewBox.model = cubeModel;
        previewMap.emplace(previewBox.getId(), std::move(previewBox));
    }

    LotGameObject SketchManager::createFinalBox(LotGameObject::Map& gameObjects) {
        // 화면 Y 움직임을 높이로 변환
        float height = (currentScreenY - secondClickScreenY) * heightScale;
        if (std::abs(height) < 0.1f)
            height = (height >= 0 ? 0.1f : -0.1f);

        // 최종 박스 생성
        glm::vec3 min = glm::vec3(std::min(firstPoint.x, secondPoint.x),
                                  baseHeight,
                                  std::min(firstPoint.z, secondPoint.z));

        glm::vec3 max = glm::vec3(std::max(firstPoint.x, secondPoint.x),
                                  baseHeight + height,
                                  std::max(firstPoint.z, secondPoint.z));

        auto finalBox = LotGameObject::createGameObject();

        glm::vec3 center = (min + max) * 0.5f;
        glm::vec3 size = max - min;

        finalBox.transform.translation = center;
        finalBox.transform.scale = glm::vec3(
            std::abs(size.x),
            std::abs(size.y),
            std::abs(size.z)
        );

        // 랜덤 색상
        finalBox.color = glm::vec3(
            0.3f + (rand() % 70) / 100.0f,
            0.3f + (rand() % 70) / 100.0f,
            0.3f + (rand() % 70) / 100.0f
        );
        finalBox.model = cubeModel;
        
        return finalBox;
    }
}