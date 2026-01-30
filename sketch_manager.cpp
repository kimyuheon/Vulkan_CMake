#include "sketch_manager.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace lot {
    SketchManager::SketchManager(LotDevice& device) : lotDevice{device} {}

    void SketchManager::startSketch() {
        state = SketchState::WaitingFirstClick;
        previewMap.clear();
        firstPointSet = false;
        viewTypeInitialized = false;
        std::cout << "=== 스케치 모드 시작 ===" << std::endl;
        std::cout << "1단계: 클릭하여 첫 점 설정" << std::endl;
    }

    void SketchManager::cancelSketch() {
        state = SketchState::Idle;
        previewMap.clear();
        firstPointSet = false;
        viewTypeInitialized = false;
        std::cout << "=== 스케치 모드 취소 ===" << std::endl;
    }

    void SketchManager::handleInput(GLFWwindow* window, const LotCamera& camera, LotGameObject::Map& gameObjects) {
        // 스케치모드 아닐 시 리턴
        if (state == SketchState::Idle) return;
        
        if (!viewTypeInitialized) {
            initializeSketchPlane(camera);
            sketchViewType = camera.getCurrentViewType();
            viewTypeInitialized = true;
        }

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

        if (state == SketchState::Idle) return;

        // 마우스 커서 가져오기
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        // 현재 마우스 위치를 월드 좌표로 변경
        bool constrainToPlane = (state != SketchState::WaitingHeight);
        currentMousePos = getMouseWorldPosition(mouseX, mouseY, windowWidth, windowHeight,
                                                camera, constrainToPlane);
        currentScreenY = mouseY;  // 현재 화면 Y 저장

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
                //baseHeight = firstPoint.y;
                firstPointSet = true;
                state = SketchState::WaitingSecondClick;
                std::cout << "2단계: 마우스 이동으로 사각형 크기 조절 후 클릭" << std::endl;
                break;
            }
            case SketchState::WaitingSecondClick:
            {
                // 두 번째 클릭 - 사각형 확정
                secondPoint = currentMousePos;
                heightStartPos = currentMousePos;
                heightStartScreenY = mouseY;  // 현재 마우스 Y를 기준점으로

                // 직접 투영으로 heightScale 계산 (1 unit 높이가 화면에서 몇 픽셀인지)
                glm::vec2 localFirst = WorldToLocal(firstPoint);
                glm::vec2 localSecond = WorldToLocal(secondPoint);
                glm::vec2 localCenter = (localFirst + localSecond) * 0.5f;

                glm::vec3 baseWorld = localToWorld(localCenter, 0.0f);
                glm::vec3 topWorld = localToWorld(localCenter, 1.0f);  // 1 unit height

                glm::mat4 projView = camera.getProjection() * camera.getView();
                glm::vec4 baseClip = projView * glm::vec4(baseWorld, 1.0f);
                glm::vec4 topClip = projView * glm::vec4(topWorld, 1.0f);

                // NDC에서 화면 Y로 변환
                float baseScreenY = ((baseClip.y / baseClip.w) * 0.5f + 0.5f) * windowHeight;
                float topScreenY = ((topClip.y / topClip.w) * 0.5f + 0.5f) * windowHeight;

                float pixelsPerUnitHeight = topScreenY - baseScreenY;

                if (std::abs(pixelsPerUnitHeight) < 0.001f) {
                    // 법선이 화면과 평행 - 기본값 사용
                    heightScale = 0.01f;
                } else {
                    heightScale = 1.0f / pixelsPerUnitHeight;
                }

                state = SketchState::WaitingHeight;
                std::cout << "3단계: 마우스를 위아래로 움직여 높이 조정 후 클릭하여 완성" << std::endl;
                std::cout << "heightScale: " << heightScale << " (pixels per unit: " << pixelsPerUnitHeight << ")" << std::endl;

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


    glm::vec3 SketchManager::getMouseWorldPosition(double mouseX, double mouseY, int windowWidth, int windowHeight,
                                            const LotCamera& camera, bool constrainToPlane) {
        float ndcX = (2.0f * static_cast<float>(mouseX)) / windowWidth - 1.0f;
        float ndcY = (2.0f * static_cast<float>(mouseY)) / windowHeight - 1.0f;

        glm::vec4 clipNear = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 clipFar = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

        glm::mat4 invProView = glm::inverse(camera.getProjection() * camera.getView());

        glm::vec4 worldNear = invProView * clipNear;
        worldNear /= worldNear.w;
        glm::vec4 worldFar = invProView * clipFar;
        worldFar /= worldFar.w;

        glm::vec3 rayOrigin = glm::vec3(worldNear);
        glm::vec3 rayDir = glm::normalize(glm::vec3(worldFar - worldNear));

        if (constrainToPlane) {
            float denom = glm::dot(rayDir, sketchPlaneNormal);

            if (std::abs(denom) < 0.0001f)
                return sketchPlaneOrigin;

            float t = glm::dot(sketchPlaneOrigin - rayOrigin, sketchPlaneNormal) / denom;

            if (t < 0.0f)
                return sketchPlaneOrigin;

            return rayOrigin + t * rayDir;
        } else {
            glm::vec2 localFirst = WorldToLocal(firstPoint);
            glm::vec2 localSecond = WorldToLocal(secondPoint);
            glm::vec2 localCenter = (localFirst + localSecond) * 0.5f;
            glm::vec3 rectCenter = localToWorld(localCenter, 0.0f);

            glm::vec3 cameraPos = camera.getPosition();
            glm::vec3 toCamera = glm::normalize(cameraPos - rectCenter);

            glm::vec3 heightPlaneNormal = glm::normalize(
                glm::cross(sketchPlaneNormal, glm::cross(toCamera, sketchPlaneNormal)));

            float denom = glm::dot(rayDir, heightPlaneNormal);

            if (std::abs(denom) < 0.0001f)
                return heightStartPos;

            float t = glm::dot(rectCenter - rayOrigin, heightPlaneNormal) / denom;

            if (t < 0.0f)
                return heightStartPos;

            glm::vec3 worldPos = rayOrigin + t * rayDir;

            glm::vec2 localPos = WorldToLocal(worldPos);
            float depth = glm::dot(worldPos - sketchPlaneOrigin, sketchPlaneNormal);
            
            return localToWorld(localCenter, depth);
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

        // 로컬 좌표계 변환
        glm::vec2 localFirst = WorldToLocal(firstPoint);
        glm::vec2 localCurrent = WorldToLocal(currentMousePos);

        glm::vec2 min = glm::vec2(std::min(localFirst.x, localCurrent.x),
                                  std::min(localFirst.y, localCurrent.y));

        glm::vec2 max = glm::vec2(std::max(localFirst.x, localCurrent.x),
                                  std::max(localFirst.y, localCurrent.y));

        glm::vec2 center = (min + max) * 0.5f;
        glm::vec2 size = max - min;

        // 너무 적을 시 예외처리
        if (size.x < 0.1f) size.x = 0.1f;
        if (size.y < 0.1f) size.y = 0.1f;

        glm::vec3 worldCenter = localToWorld(center, 0);

        // 회전: 로컬 Y축이 법선(높이) 방향을 향하도록
        glm::mat3 rotationMat = glm::mat3(sketchPlaneRight, sketchPlaneNormal, sketchPlaneUp);
        glm::quat rotation = glm::quat_cast(glm::mat4(rotationMat));

        // 얇은 박스로 바닥면 프리뷰 (X: right, Y: 높이, Z: up)
        auto previewBox = LotGameObject::createGameObject();
        previewBox.transform.translation = worldCenter;
        previewBox.transform.rotation = rotation;
        previewBox.transform.scale = glm::vec3(size.x, 0.05f, size.y);  // 얇은 바닥면
        previewBox.color = glm::vec3(0.2f, 0.8f, 1.0f);  // 하늘색
        previewBox.model = cubeModel;
        previewMap.emplace(previewBox.getId(), std::move(previewBox));
    }

    void SketchManager::updateBoxPreview() {
        previewMap.clear();

        glm::vec2 localFirst = WorldToLocal(firstPoint);
        glm::vec2 localCurrent = WorldToLocal(secondPoint);

        glm::vec2 min = glm::vec2(std::min(localFirst.x, localCurrent.x),
                                  std::min(localFirst.y, localCurrent.y));

        glm::vec2 max = glm::vec2(std::max(localFirst.x, localCurrent.x),
                                  std::max(localFirst.y, localCurrent.y));

        glm::vec2 center = (min + max) * 0.5f;
        glm::vec2 size = max - min;

        // 화면 Y 변화량을 높이로 변환 (위로 움직이면 양수)
        float height = static_cast<float>(currentScreenY - heightStartScreenY) * heightScale;

        const float minHeight = 0.1f;
        if (std::abs(height) < minHeight)
            height = (height >= 0 ? minHeight : -minHeight);

        // 너무 적을 시 예외처리
        if (size.x < 0.1f) size.x = 0.1f;
        if (size.y < 0.1f) size.y = 0.1f;

        float depthCenter = height * 0.5f;
        glm::vec3 worldCenter = localToWorld(center, depthCenter);

        // 회전: 로컬 Y축이 법선(높이) 방향을 향하도록
        glm::mat3 rotationMat = glm::mat3(sketchPlaneRight, sketchPlaneNormal, sketchPlaneUp);
        glm::quat rotation = glm::quat_cast(glm::mat4(rotationMat));

        // 박스 프리뷰 (X: right, Y: 높이, Z: up)
        auto previewBox = LotGameObject::createGameObject();
        previewBox.transform.translation = worldCenter;
        previewBox.transform.rotation = rotation;
        previewBox.transform.scale = glm::vec3(
            std::abs(size.x),
            std::abs(height),
            std::abs(size.y)
        );
        previewBox.color = glm::vec3(0.2f, 0.8f, 1.0f);
        previewBox.model = cubeModel;
        previewMap.emplace(previewBox.getId(), std::move(previewBox));
    }

    LotGameObject SketchManager::createFinalBox(LotGameObject::Map& gameObjects) {
        glm::vec2 localFirst = WorldToLocal(firstPoint);
        glm::vec2 localCurrent = WorldToLocal(secondPoint);

        // 최종 박스 생성
        glm::vec2 min = glm::vec2(std::min(localFirst.x, localCurrent.x),
                                  std::min(localFirst.y, localCurrent.y));

        glm::vec2 max = glm::vec2(std::max(localFirst.x, localCurrent.x),
                                  std::max(localFirst.y, localCurrent.y));

        glm::vec2 center = (min + max) * 0.5f;
        glm::vec2 size = max - min;

        // 화면 Y 변화량을 높이로 변환 (위로 움직이면 양수)
        float height = static_cast<float>(currentScreenY - heightStartScreenY) * heightScale;

        const float minHeight = 0.1f;
        if (std::abs(height) < minHeight)
            height = (height >= 0 ? minHeight : -minHeight);

        // 너무 적을 시 예외처리
        if (size.x < 0.1f) size.x = 0.1f;
        if (size.y < 0.1f) size.y = 0.1f;

        float depthCenter = height * 0.5f;
        glm::vec3 worldCenter = localToWorld(center, depthCenter);

        // 회전: 로컬 Y축이 법선(높이) 방향을 향하도록
        glm::mat3 rotationMat = glm::mat3(sketchPlaneRight, sketchPlaneNormal, sketchPlaneUp);
        glm::quat rotation = glm::quat_cast(glm::mat4(rotationMat));

        auto finalBox = LotGameObject::createGameObject();
        finalBox.transform.translation = worldCenter;
        finalBox.transform.rotation = rotation;
        finalBox.transform.scale = glm::vec3(
            std::abs(size.x),
            std::abs(height),
            std::abs(size.y)
        );

        // 랜덤 색상
        finalBox.color = glm::vec3(
            0.3f + (rand() % 70) / 100.0f,
            0.3f + (rand() % 70) / 100.0f,
            0.3f + (rand() % 70) / 100.0f
        );
        finalBox.model = cubeModel;
        
        std::cout << "생성된 박스:" << std::endl;
        std::cout << "  평면: ";
        switch (activeSketchPlane) {
            case SketchPlane::XY: std::cout << "XY"; break;
            case SketchPlane::YZ: std::cout << "YZ"; break;
            case SketchPlane::XZ: std::cout << "XZ"; break;
        }
        std::cout << std::endl;
        std::cout << "  위치: (" << worldCenter.x << ", " << worldCenter.y << ", " 
                  << worldCenter.z << ")" << std::endl;
        std::cout << "  크기: (" << size.x << ", " << size.y << ", " 
                  << std::abs(height) << ")" << std::endl;

        return finalBox;
    }

    SketchManager::SketchPlane SketchManager::determineSketchPlane(const LotCamera& camera) {
        glm::mat4 invView = camera.getInverseView();
        glm::vec3 cameraForward = -glm::normalize(glm::vec3(invView[2]));

        glm::vec3 worldX = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 worldY = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 worldZ = glm::vec3(0.0f, 0.0f, 1.0f);

        float dotX = std::abs(glm::dot(cameraForward, worldX));
        float dotY = std::abs(glm::dot(cameraForward, worldY));
        float dotZ = std::abs(glm::dot(cameraForward, worldZ));

        if (dotX > dotY && dotX > dotZ)
            return SketchPlane::YZ;
        else if (dotY > dotX && dotY > dotZ)
            return SketchPlane::XZ;
        else
            return SketchPlane::XY;
    }

    void SketchManager::getPlaneVectors(SketchPlane plane, glm::vec3& right, glm::vec3& up, glm::vec3& normal) {
        // 벡터들이 오른손 좌표계를 형성하도록: Right × Normal = Up
        switch (plane) {
            case SketchPlane::XY:
                // Front view: (1,0,0) × (0,0,-1) = (0,1,0) ✓
                right = glm::vec3(1.0f, 0.0f, 0.0f);
                up = glm::vec3(0.0f, 1.0f, 0.0f);
                normal = glm::vec3(0.0f, 0.0f, -1.0f);
                break;
            case SketchPlane::YZ:
                // Right view: (0,0,1) × (1,0,0) = (0,1,0) ✓
                right = glm::vec3(0.0f, 0.0f, 1.0f);
                up = glm::vec3(0.0f, 1.0f, 0.0f);
                normal = glm::vec3(1.0f, 0.0f, 0.0f);
                break;
            case SketchPlane::XZ:
                // Top view: (1,0,0) × (0,1,0) = (0,0,1) ✓
                right = glm::vec3(1.0f, 0.0f, 0.0f);
                up = glm::vec3(0.0f, 0.0f, 1.0f);
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
                break;
        }
    }

    void SketchManager::initializeSketchPlane(const LotCamera& camera) {
        // 뷰 타입이 설정되어 있으면 그에 따라 스케치 평면 결정 (회전해도 유지)
        LotCamera::CadViewType viewType = camera.getCurrentViewType();

        switch (viewType) {
            case LotCamera::CadViewType::Front:
                activeSketchPlane = SketchPlane::XY;
                break;
            case LotCamera::CadViewType::Top:
                activeSketchPlane = SketchPlane::XZ;
                break;
            case LotCamera::CadViewType::Right:
                activeSketchPlane = SketchPlane::YZ;
                break;
            default:
                // Isometric이나 기타 뷰에서는 카메라 방향으로 결정
                activeSketchPlane = determineSketchPlane(camera);
                break;
        }

        getPlaneVectors(activeSketchPlane, sketchPlaneRight, sketchPlaneUp, sketchPlaneNormal);

        if (camera.getCadMode()) {
            sketchPlaneOrigin = camera.getTarget();
        } else {
            sketchPlaneOrigin = glm::vec3(0.0f);
        }

        std::cout << "스케치 평면: ";
        switch (activeSketchPlane) {
            case SketchPlane::XY: std::cout << "XY 평면 (정면도)" << std::endl; break;
            case SketchPlane::YZ: std::cout << "YZ 평면 (우측면도)" << std::endl; break;
            case SketchPlane::XZ: std::cout << "XZ 평면 (평면도)" << std::endl; break;
        }
    }

    glm::vec3 SketchManager::localToWorld(const glm::vec2& localPoint, float depth) {
        return sketchPlaneOrigin + 
               localPoint.x * sketchPlaneRight + 
               localPoint.y * sketchPlaneUp + 
               depth * sketchPlaneNormal;
    }

    glm::vec2 SketchManager::WorldToLocal(const glm::vec3& worldPoint) {
        glm::vec3 offset = worldPoint - sketchPlaneOrigin;
        float x = glm::dot(offset, sketchPlaneRight);
        float y = glm::dot(offset, sketchPlaneUp);
        return glm::vec2(x, y);
    }
}