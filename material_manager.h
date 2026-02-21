#pragma once

#include "lot_device.h"
#include "lot_texture.h"
#include "lot_game_object.h"

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace lot {
    class MaterialManager {
        public:
            MaterialManager(LotDevice& device);
            ~MaterialManager();

            MaterialManager(const MaterialManager&) = delete;
            MaterialManager& operator=(const MaterialManager&) = delete;

            // textures 폴더 스캔하여 자동으로 로드
            void loadMaterialsFromFolder(const std::string& folderPath = "textures");
            // 개별 텍스처 로드
            void loadMaterial(const std::string& name, const std::string& filepath);
            // 선택된 객체에 재질 적용
            void applyMaterialToSelected(const std::string materialName, 
                                         LotGameObject::Map& gameObjects);
            // 선택한 객체의 재질 제거
            void removeMaterialFromSelected(LotGameObject::Map& gameObjects);
            // 사용 가능한 재질 이름 목록
            std::vector<std::string> getAvailableMaterials() const;
            // 이름으로 재질 포인터 가져오기
            LotTexture* getTexture(const std::string& name) const;
            // 기본 재질(하얀색) 가져오기
            LotTexture* getDefaultTexture() const { return defaultTexture.get(); }

            bool hasMaterials() const { return !textures.empty(); }

        private:
            LotDevice& lotDevice;
            std::map<std::string, std::unique_ptr<LotTexture>> textures;
            std::unique_ptr<LotTexture> defaultTexture;
    };
}