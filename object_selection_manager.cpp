#include "object_selection_manager.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <limits>
#include <iostream>

namespace lot {

    void ObjectSelectionManager::handleMouseClick(GLFWwindow* window,
                                                const LotCamera& camera,
                                                std::vector<LotGameObject>& gameObjects) {

        // 카메라 저장 (투영 계산에 필요)
        currentCamera = &camera;

        int leftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

        if (leftState == GLFW_PRESS && !leftMousePressed) {
            leftMousePressed = true;

            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            // 마우스 좌표 저장
            lastMouseX = mouseX;
            lastMouseY = mouseY;

            glfwGetWindowSize(window, &windowWidth, &windowHeight);

            Ray ray = screenToWorldRay(mouseX, mouseY, windowWidth, windowHeight, camera);
            LotGameObject* hitObject = findIntersectedObject(ray, gameObjects);

            bool ctrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                              glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

            if (hitObject != nullptr) {
                if (ctrlPressed) {
                    if (hitObject->isSelected) {
                        deselectObject(hitObject->getId(), gameObjects);
                    } else {
                        selectObject(hitObject->getId(), gameObjects, true);
                    }
                } else {
                    clearAllSelections(gameObjects);
                    selectObject(hitObject->getId(), gameObjects, false);
                }
            } else if (!ctrlPressed) {
                clearAllSelections(gameObjects);
            }
        } else if (leftState == GLFW_RELEASE) {
            leftMousePressed = false;
        }
    }

    void ObjectSelectionManager::clearAllSelections(std::vector<LotGameObject>& gameObjects) {
        for (auto& obj : gameObjects) {
            obj.isSelected = false;
        }
        selectedObjectIds.clear();
    }

    bool ObjectSelectionManager::isObjectSelected(const LotGameObject& object) const {
        return selectedObjectIds.find(object.getId()) != selectedObjectIds.end();
    }

    void ObjectSelectionManager::selectObject(LotGameObject::id_t objectId,
                                            std::vector<LotGameObject>& gameObjects,
                                            bool multiSelect) {
        if (!multiSelect) {
            clearAllSelections(gameObjects);
        }

        for (auto& obj : gameObjects) {
            if (obj.getId() == objectId) {
                obj.isSelected = true;
                selectedObjectIds.insert(objectId);
                break;
            }
        }
    }

    void ObjectSelectionManager::deselectObject(LotGameObject::id_t objectId,
                                              std::vector<LotGameObject>& gameObjects) {
        for (auto& obj : gameObjects) {
            if (obj.getId() == objectId) {
                obj.isSelected = false;
                selectedObjectIds.erase(objectId);
                break;
            }
        }
    }

    ObjectSelectionManager::Ray ObjectSelectionManager::screenToWorldRay(
        double mouseX, double mouseY, int windowWidth, int windowHeight,
        const LotCamera& camera) {

        // 1. 화면 좌표 → 정규화된 디바이스 좌표 (NDC) 변환
        // Vulkan NDC: x[-1,1], y[-1(위),1(아래)]
        // 화면 좌표: x[0(왼),width(오)], y[0(위),height(아래)]
        float x = (float)((2.0f * mouseX) / windowWidth - 1.0f);
        float y = (float)((2.0f * mouseY) / windowHeight - 1.0f);

        // 2. 카메라 변환 매트릭스 준비
        glm::mat4 viewMatrix = camera.getView();        // 월드 → 뷰 공간 변환
        glm::mat4 projectionMatrix = camera.getProjection(); // 뷰 → 클립 공간 변환

        // 3. 카메라 월드 위치 추출
        // 뷰 매트릭스 역변환의 네 번째 열이 카메라의 월드 위치
        glm::mat4 invView = glm::inverse(viewMatrix);
        glm::vec3 cameraPos = glm::vec3(invView[3][0], invView[3][1], invView[3][2]);

        // 4. Near/Far 평면에서의 클립 공간 좌표 생성
        // z = -1 (near plane), z = 1 (far plane)
        glm::vec4 rayClipNear = glm::vec4(x, y, -1.0f, 1.0f);
        glm::vec4 rayClipFar = glm::vec4(x, y, 1.0f, 1.0f);

        // 5. 클립 공간 → 뷰 공간 역변환 (투영 역변환)
        // P^-1 * clip_coords = view_coords
        glm::mat4 invProjection = glm::inverse(projectionMatrix);
        glm::vec4 rayViewNear = invProjection * rayClipNear;
        glm::vec4 rayViewFar = invProjection * rayClipFar;

        // 6. 동차 좌표 정규화 (Perspective Division 역과정)
        // view_coords = homogeneous_coords / w
        rayViewNear /= rayViewNear.w;
        rayViewFar /= rayViewFar.w;

        // 7. 뷰 공간 → 월드 공간 역변환
        // V^-1 * view_coords = world_coords
        glm::vec4 rayWorldNear = invView * rayViewNear;
        glm::vec4 rayWorldFar = invView * rayViewFar;

        // 8. 레이 방향 벡터 계산 및 정규화
        // Ray Direction = normalize(far_point - near_point)
        glm::vec3 worldNear = glm::vec3(rayWorldNear);
        glm::vec3 worldFar = glm::vec3(rayWorldFar);
        glm::vec3 rayDirection = glm::normalize(worldFar - worldNear);

        // 9. 최종 레이 생성: Ray(origin, direction)
        Ray ray;
        ray.origin = cameraPos;      // 카메라 위치에서 시작
        ray.direction = rayDirection; // 정규화된 방향 벡터

        return ray;
    }

    bool ObjectSelectionManager::rayIntersectsBoundingBox(const Ray& ray, const BoundingBox& bbox) {
        // 표준 레이-AABB 교차 검사 (slab method)
        // 0에 가까운 방향 성분들을 보정
        const float EPSILON = 1e-8f;
    
        glm::vec3 safeDir = ray.direction;
        for (int i = 0; i < 3; i++) {
            if (abs(safeDir[i]) < EPSILON) {
                safeDir[i] = safeDir[i] >= 0 ? EPSILON : -EPSILON;
            }
        }

        glm::vec3 invDir = glm::vec3(1.0f) / safeDir;

        glm::vec3 t1 = (bbox.min - ray.origin) * invDir;
        glm::vec3 t2 = (bbox.max - ray.origin) * invDir;

        glm::vec3 tmin = glm::min(t1, t2);
        glm::vec3 tmax = glm::max(t1, t2);

        float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
        float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

        bool intersects = tNear <= tFar && tFar >= 0.0f;

        return intersects;
    }

    bool ObjectSelectionManager::rayIntersectsBoundingBoxWithDistance(const Ray& ray, const BoundingBox& bbox, float& distance) {
        // Slab Method: AABB-Ray 교차 검사 알고리즘
        const float EPSILON = 1e-8f;

        float tmin = 0.0f;  // 레이 시작점을 0으로 (음수 거리는 무시)
        float tmax = std::numeric_limits<float>::max();

        // 각 축(X, Y, Z)에 대해 slab 교차 검사
        for (int i = 0; i < 3; i++) {
            if (std::abs(ray.direction[i]) < EPSILON) {
                // 레이가 축과 평행한 경우
                if (ray.origin[i] < bbox.min[i] || ray.origin[i] > bbox.max[i]) {
                    return false;
                }
            } else {
                float t1 = (bbox.min[i] - ray.origin[i]) / ray.direction[i];
                float t2 = (bbox.max[i] - ray.origin[i]) / ray.direction[i];

                if (t1 > t2) std::swap(t1, t2);

                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);

                if (tmin > tmax) {
                    return false;
                }
            }
        }

        // tmax > tmin이고 tmax > 0이면 교차
        if (tmax > tmin && tmax > 0.0f) {
            distance = tmin;
            return true;
        }

        return false;
    }

    ObjectSelectionManager::BoundingBox ObjectSelectionManager::calculateBoundingBox(const LotGameObject& object) {
        if (!object.model) {
            BoundingBox bbox;
            bbox.min = object.transform.translation - glm::vec3(0.1f);
            bbox.max = object.transform.translation + glm::vec3(0.1f);
            return bbox;
        }

        glm::vec3 scale = object.transform.scale;
        glm::vec3 translation = object.transform.translation;

        // 큐브의 실제 크기는 1x1x1이므로, 스케일을 그대로 반영
        glm::vec3 halfSize = glm::vec3(0.5f) * scale;

        BoundingBox bbox;
        bbox.min = translation - halfSize;
        bbox.max = translation + halfSize;


        return bbox;
    }

    LotGameObject* ObjectSelectionManager::findIntersectedObject(const Ray& ray,
                                                              std::vector<LotGameObject>& gameObjects) {
        return findClosestObjectInScreenSpace(ray, gameObjects);
    }

    LotGameObject* ObjectSelectionManager::findClosestObjectInScreenSpace(const Ray& ray,
                                                                        std::vector<LotGameObject>& gameObjects) {
        LotGameObject* closestObject = nullptr;
        float bestSelectionScore = std::numeric_limits<float>::max();

        // 마우스 위치 (화면 좌표)
        glm::vec2 mouseScreenPos = glm::vec2(lastMouseX, lastMouseY);


        for (auto& obj : gameObjects) {
            if (!obj.model) continue;

            bool isSelected = false;
            float intersectionDistance = 0.0f;

            // 복잡한 메시인 경우 정밀한 레이-메시 교차 검사
            if (hasComplexGeometry(obj)) {
                isSelected = rayIntersectsMesh(ray, obj, intersectionDistance);
            }
            // 단순한 객체(큐브 등)도 레이-바운딩박스 교차 검사 사용
            else {
                BoundingBox bbox = calculateBoundingBox(obj);
                isSelected = rayIntersectsBoundingBoxWithDistance(ray, bbox, intersectionDistance);
            }

            if (isSelected) {
                // 선택 점수는 교차 거리로 계산
                float selectionScore = intersectionDistance;

                if (selectionScore < bestSelectionScore) {
                    bestSelectionScore = selectionScore;
                    closestObject = &obj;
                }
            }
        }

        return closestObject;
    }

    glm::vec2 ObjectSelectionManager::projectToScreen(const glm::vec3& worldPos, const Ray& ray) {
        if (!currentCamera) {
            // 카메라 정보가 없으면 기본 투영 사용
            return glm::vec2(windowWidth / 2.0f, windowHeight / 2.0f);
        }

        // 월드 좌표를 동차 좌표로 변환
        glm::vec4 worldPoint = glm::vec4(worldPos, 1.0f);

        // 뷰 변환
        glm::vec4 viewPoint = currentCamera->getView() * worldPoint;

        // 투영 변환
        glm::vec4 clipPoint = currentCamera->getProjection() * viewPoint;

        // 동차 나누기 (원근 나누기)
        if (clipPoint.w != 0.0f) {
            clipPoint /= clipPoint.w;
        }

        // NDC(-1~1)를 화면 좌표로 변환
        float screenX = (clipPoint.x + 1.0f) * 0.5f * windowWidth;
        float screenY = (1.0f - clipPoint.y) * 0.5f * windowHeight;  // Y축 뒤집기

        return glm::vec2(screenX, screenY);
    }

    float ObjectSelectionManager::calculateScreenRadius(const LotGameObject& obj, const Ray& ray) {
        // 객체와 카메라 사이의 거리
        float distance = glm::length(obj.transform.translation - ray.origin);

        // 객체의 월드 크기
        float worldSize = glm::max(glm::max(obj.transform.scale.x, obj.transform.scale.y), obj.transform.scale.z);

        // 화면상 반지름 (거리에 반비례) - 적절한 크기로 조정
        float screenRadius = (worldSize * 200.0f) / distance;

        // 디버그 출력
        std::cout << "    Radius calc: worldSize=" << worldSize << ", distance=" << distance
                  << ", calculated=" << screenRadius << std::endl;

        // 최소/최대 반지름 제한 (적절한 크기로 설정)
        return glm::clamp(screenRadius, 30.0f, 80.0f);
    }

    ObjectSelectionManager::ScreenRect ObjectSelectionManager::projectBoundingBoxToScreen(const BoundingBox& bbox) {
        if (!currentCamera) {
            return {0, 0, static_cast<float>(windowWidth), static_cast<float>(windowHeight)};
        }

        // 3D 바운딩박스의 8개 꼭짓점 생성
        std::vector<glm::vec3> corners = {
            {bbox.min.x, bbox.min.y, bbox.min.z},
            {bbox.max.x, bbox.min.y, bbox.min.z},
            {bbox.min.x, bbox.max.y, bbox.min.z},
            {bbox.max.x, bbox.max.y, bbox.min.z},
            {bbox.min.x, bbox.min.y, bbox.max.z},
            {bbox.max.x, bbox.min.y, bbox.max.z},
            {bbox.min.x, bbox.max.y, bbox.max.z},
            {bbox.max.x, bbox.max.y, bbox.max.z}
        };

        float minScreenX = std::numeric_limits<float>::max();
        float maxScreenX = std::numeric_limits<float>::lowest();
        float minScreenY = std::numeric_limits<float>::max();
        float maxScreenY = std::numeric_limits<float>::lowest();

        // 모든 꼭짓점을 화면에 투영하고 최소/최대값 찾기
        for (size_t i = 0; i < corners.size(); ++i) {
            glm::vec2 screenPos = projectToScreen(corners[i], {});

            std::cout << "    Corner " << i << " world(" << corners[i].x << "," << corners[i].y << "," << corners[i].z
                      << ") -> screen(" << screenPos.x << "," << screenPos.y << ")" << std::endl;

            minScreenX = std::min(minScreenX, screenPos.x);
            maxScreenX = std::max(maxScreenX, screenPos.x);
            minScreenY = std::min(minScreenY, screenPos.y);
            maxScreenY = std::max(maxScreenY, screenPos.y);
        }

        // 투영 오차를 고려해 바운딩박스를 적당히 확장
        float margin = 10.0f;
        return {minScreenX - margin, minScreenY - margin, maxScreenX + margin, maxScreenY + margin};
    }

    bool ObjectSelectionManager::isPointInsideRect(const glm::vec2& point, const ScreenRect& rect) {
        return point.x >= rect.minX && point.x <= rect.maxX &&
               point.y >= rect.minY && point.y <= rect.maxY;
    }

    bool ObjectSelectionManager::rayIntersectsTriangle(const Ray& ray, const Triangle& triangle, float& t) {
        // Möller-Trumbore 레이-삼각형 교차 알고리즘
        // 원리: 레이와 삼각형의 교차점을 중심좌표(barycentric coordinates)로 계산
        // 삼각형 위의 점 P = v0 + u*edge1 + v*edge2 (u,v는 중심좌표, u+v≤1, u≥0, v≥0)

        const float EPSILON = 1e-8f;

        // 삼각형의 두 모서리 벡터 계산
        glm::vec3 edge1 = triangle.v1 - triangle.v0; // v0 → v1
        glm::vec3 edge2 = triangle.v2 - triangle.v0; // v0 → v2

        // P벡터 계산: P = ray.direction × edge2
        glm::vec3 h = glm::cross(ray.direction, edge2);

        // 행렬식 계산: a = edge1 · P
        // 이 값이 0에 가까우면 레이가 삼각형 평면과 평행
        float a = glm::dot(edge1, h);

        if (a > -EPSILON && a < EPSILON) {
            return false; // 레이가 삼각형과 평행 → 교차하지 않음
        }

        // 역행렬식 계산 (1/det)
        float f = 1.0f / a;

        // T벡터 계산: T = ray.origin - v0
        glm::vec3 s = ray.origin - triangle.v0;

        // 첫 번째 중심좌표 u 계산: u = f * (T · P)
        float u = f * glm::dot(s, h);

        // u가 [0,1] 범위를 벗어나면 삼각형 밖
        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        // Q벡터 계산: Q = T × edge1
        glm::vec3 q = glm::cross(s, edge1);

        // 두 번째 중심좌표 v 계산: v = f * (ray.direction · Q)
        float v = f * glm::dot(ray.direction, q);

        // v ≥ 0 이고 u + v ≤ 1 이어야 삼각형 내부
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        // 교차 거리 t 계산: t = f * (edge2 · Q)
        t = f * glm::dot(edge2, q);

        // t > EPSILON: 레이가 앞방향으로 교차해야 유효
        return t > EPSILON;
    }

    bool ObjectSelectionManager::rayIntersectsMesh(const Ray& ray, const LotGameObject& obj, float& distance) {
        if (!obj.model) return false;

        const auto& vertices = obj.model->getVertices();
        const auto& indices = obj.model->getIndices();

        // 인덱스 버퍼가 없으면 바운딩박스로 대체
        if (!obj.model->hasIndices() || indices.empty()) {
            return rayIntersectsBoundingBoxWithDistance(ray, calculateBoundingBox(obj), distance);
        }

        float closestDistance = std::numeric_limits<float>::max();
        bool hitFound = false;

        // 모든 삼각형 검사
        for (size_t i = 0; i < indices.size(); i += 3) {
            // 삼각형 버텍스 가져오기
            glm::vec3 v0 = vertices[indices[i]].position;
            glm::vec3 v1 = vertices[indices[i + 1]].position;
            glm::vec3 v2 = vertices[indices[i + 2]].position;

            // 객체의 트랜스폼 적용
            glm::mat4 modelMatrix = obj.transform.mat4();
            v0 = glm::vec3(modelMatrix * glm::vec4(v0, 1.0f));
            v1 = glm::vec3(modelMatrix * glm::vec4(v1, 1.0f));
            v2 = glm::vec3(modelMatrix * glm::vec4(v2, 1.0f));

            // 레이-삼각형 교차 검사
            Triangle tri = {v0, v1, v2};
            float t;
            if (rayIntersectsTriangle(ray, tri, t)) {
                if (t < closestDistance) {
                    closestDistance = t;
                    hitFound = true;
                }
            }
        }

        if (hitFound) {
            distance = closestDistance;
            return true;
        }

        return false;
    }

    bool ObjectSelectionManager::hasComplexGeometry(const LotGameObject& obj) {
        if (!obj.model) return false;

        const auto& indices = obj.model->getIndices();

        // 삼각형 개수가 많으면 복잡한 메시로 판단
        // 큐브는 12개 삼각형(36개 인덱스), 이보다 많으면 복잡한 것으로 간주
        const size_t COMPLEX_MESH_THRESHOLD = 100; // 100개 삼각형 이상

        if (!obj.model->hasIndices() || indices.empty()) {
            return false; // 인덱스 없으면 단순한 것으로 간주
        }

        size_t triangleCount = indices.size() / 3;
        return triangleCount > COMPLEX_MESH_THRESHOLD;
    }
}