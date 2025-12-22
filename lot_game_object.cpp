#include "lot_game_object.h"

namespace lot {
    // Transformcomponent
    glm::mat4 Transformcomponent::mat4() const {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return T * R * S;
    }

    glm::mat4 Transformcomponent::normalMatrix() const {
        const float scaleX = scale.x;
        const float scaleY = scale.y;
        const float scaleZ = scale.z;

        glm::vec3 invScale{1.f / scaleX, 1.f / scaleY, 1.f / scaleZ};

        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 invS = glm::scale(glm::mat4(1.0f), invScale);

        return R * invS;
    }

    void Transformcomponent::setRotationEuler(float x, float y, float z) {
        glm::quat qx = glm::angleAxis(x, glm::vec3(1, 0, 0));
        glm::quat qy = glm::angleAxis(y, glm::vec3(0, 1, 0));
        glm::quat qz = glm::angleAxis(z, glm::vec3(0, 0, 1));
        rotation = qy * qx * qz;  // Y-X-Z 순서
    }

    void Transformcomponent::rotateAroundAxis(float angle, const glm::vec3& axis) {
        glm::quat rotQuat = glm::angleAxis(angle, axis);
        rotation = rotation * rotQuat;
    }

    //
    LotGameObject LotGameObject::makePointLight(float intensity, float radius, glm::vec3 color) {
        LotGameObject obj = LotGameObject::createGameObject();
        obj.color = color;
        obj.transform.scale.x = radius;
        obj.pointLight = std::make_unique<PointLightComponent>();
        obj.pointLight->lightIntensity = intensity;
        return obj;
    }
}