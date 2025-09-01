#pragma once

#include "lot_model.h"

#include <memory>

namespace lot {
    struct Transform2dcomponent {
        glm::vec2 translation{};
        glm::vec2 scale{1.f, 1.f};
        float rotation;

        glm::mat2 mat2(){
            const float s = glm::sin(rotation);
            const float c = glm::cos(rotation);
            glm::mat2 rotMatrix{{c, s}, {-s, c}};

            glm::mat2 scaleMat{{scale.x, .0f}, {.0f, scale.y}};
            return rotMatrix * scaleMat;
        }
    };

    class LotGameObject {
        public:
            using id_t = unsigned int;

            static LotGameObject createGameObject() {
                static id_t currentId = 0;
                return LotGameObject{currentId++};
            }

            LotGameObject(const LotGameObject &) = delete;
            LotGameObject& operator=(const LotGameObject &) = delete;
            LotGameObject(LotGameObject &&) = default;
            LotGameObject& operator=(LotGameObject &&) = default;

            id_t getId() { return id; }

            std::shared_ptr<LotModel> model{};
            glm::vec3 color{};
            Transform2dcomponent transform2d{};

        private:
            LotGameObject(id_t objId) : id(objId) {}

            id_t id;
    };
} // namespace lot