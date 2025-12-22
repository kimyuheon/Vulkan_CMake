#pragma once

#include "lot_game_object.h"
#include "lot_camera.h"
#include "lot_window.h"

#include <glm/glm.hpp>
#include <vector>
#include <set>

namespace lot {
    class ObjectSelectionManager {
    public:
        ObjectSelectionManager() = default;
        ~ObjectSelectionManager() = default;

        struct Ray {
            glm::vec3 origin;
            glm::vec3 direction;
        };

        struct BoundingBox {
            glm::vec3 min;
            glm::vec3 max;
        };

        struct ScreenRect {
            float minX, minY, maxX, maxY;
        };

        struct Triangle {
            glm::vec3 v0, v1, v2;
        };

        void handleMouseClick(GLFWwindow* window,
                            const LotCamera& camera,
                            LotGameObject::Map& gameObjects);

        void clearAllSelections(LotGameObject::Map& gameObjects);

        bool isObjectSelected(const LotGameObject& object) const;

        const std::set<LotGameObject::id_t>& getSelectedObjects() const {
            return selectedObjectIds;
        }

        void selectObject(LotGameObject::id_t objectId,
                         LotGameObject::Map& gameObjects,
                         bool multiSelect = false);

        void deselectObject(LotGameObject::id_t objectId, LotGameObject::Map& gameObjects);

    private:
        Ray screenToWorldRay(double mouseX, double mouseY,
                           int windowWidth, int windowHeight,
                           const LotCamera& camera);

        bool rayIntersectsBoundingBox(const Ray& ray, const BoundingBox& bbox);

        bool rayIntersectsBoundingBoxWithDistance(const Ray& ray, const BoundingBox& bbox, float& distance);

        BoundingBox calculateBoundingBox(const LotGameObject& object);

        LotGameObject* findIntersectedObject(const Ray& ray, LotGameObject::Map& gameObjects);

        LotGameObject* findClosestObjectInScreenSpace(const Ray& ray, LotGameObject::Map& gameObjects);

        glm::vec2 projectToScreen(const glm::vec3& worldPos, const Ray& ray);

        float calculateScreenRadius(const LotGameObject& obj, const Ray& ray);

        ScreenRect projectBoundingBoxToScreen(const BoundingBox& bbox);

        bool isPointInsideRect(const glm::vec2& point, const ScreenRect& rect);

        bool rayIntersectsTriangle(const Ray& ray, const Triangle& triangle, float& t);

        bool rayIntersectsMesh(const Ray& ray, const LotGameObject& obj, float& distance);

        bool hasComplexGeometry(const LotGameObject& obj);

        std::set<LotGameObject::id_t> selectedObjectIds;
        bool leftMousePressed = false;
        double lastMouseX = 0.0;
        double lastMouseY = 0.0;
        const LotCamera* currentCamera = nullptr;
        int windowWidth = 800;
        int windowHeight = 600;
    };
}