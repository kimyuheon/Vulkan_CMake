#include "material_manager.h"

#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace lot {
    MaterialManager::MaterialManager(LotDevice& device) 
    : lotDevice{device} {
        defaultTexture = LotTexture::createDefaultTexture(lotDevice);
        std::cout << "MaterialManager] Initialized with default texture" << std::endl;
    }

    MaterialManager::~MaterialManager() {
        textures.clear();
        defaultTexture.reset();
        std::cout << "[MaterialManager] Destroyed" << std::endl;
    }

    // textures 폴더 스캔하여 자동으로 로드
    void MaterialManager::loadMaterialsFromFolder(const std::string& folderPath) {
        if (!fs::exists(folderPath)) {
            std::cout << "MaterialManager] Folder not found:" << folderPath
                      << "(create it and add .jpg/.png files)" <<  std::endl;
            return;
        }

        if (!fs::is_directory(folderPath)) {
            std::cerr << "MaterialManager] Path is not a directory: " 
                      << folderPath << std::endl;
            return;
        }

        int loadedCount = 0;
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (!entry.is_regular_file())
                continue;

            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
                ext == ".bmp" || ext == ".tga") {
                std::string name = entry.path().stem().string();
                std::string filepath = entry.path().string();
                loadMaterial(name, filepath);
                loadedCount++;
            }

            if (loadedCount == 0)
                std::cout << "[MaterialManager] No texture files found in " << folderPath << std::endl;
            else 
                std::cout << "[MaterialManager] Loaded " << loadedCount << "Material(s)" << std::endl;
        }
    }

    // 개별 텍스처 로드
    void MaterialManager::loadMaterial(const std::string& name, const std::string& filepath) {
        try {
            auto texture = LotTexture::createTextureFromFile(lotDevice, filepath);
            textures[name] = std::move(texture);
            std::cerr << "[MaterialManager] Loaded: " << name << " <- " << filepath << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[MaterialManager] Failed to load  '" << name << "': " << e.what() << std::endl;
        }

    }
    // 선택된 객체에 재질 적용
    void MaterialManager::applyMaterialToSelected(const std::string materialName, 
                                                  LotGameObject::Map& gameObjects) {
        auto it = textures.find(materialName);
        if (it == textures.end()) {
            std::cerr << "[MaterialManager] Texture not found: " << materialName << std::endl;
            return;
        }

        int appliedCount = 0;
        for (auto& kv : gameObjects) {
            auto& obj = kv.second;
            if (obj.isSelected && obj.model) {
                obj.textureName = materialName;
                obj.texture = it->second.get();
                appliedCount++;
            }
        }

        if (appliedCount > 0)
                std::cout << "[MaterialManager] Applied '" << materialName 
                          << "' to " << appliedCount << " object(s)" << std::endl;
        else 
            std::cout << "[MaterialManager] No selected objects to apply material " << std::endl;
    }

    // 선택한 객체의 재질 제거
    void MaterialManager::removeMaterialFromSelected(LotGameObject::Map& gameObjects) {
        int removeCount = 0;
        for (auto& kv : gameObjects) {
            auto& obj = kv.second;
            if (obj.isSelected) {
                obj.textureName = "";
                obj.texture = nullptr;
                removeCount++;
            }
        }
        if (removeCount == 0)
            std::cout << "[MaterialManager] Removed material from "  
                      << removeCount << " object(s)" << std::endl;
    }

    // 사용 가능한 재질 이름 목록
    std::vector<std::string> MaterialManager::getAvailableMaterials() const {
        std::vector<std::string> names;
        names.reserve(textures.size());
        for (const auto& kv : textures) 
            names.push_back(kv.first);

        return names;
    }

    // 이름으로 재질 포인터 가져오기
    LotTexture* MaterialManager::getTexture(const std::string& name) const {
        auto it = textures.find(name);
        if (it != textures.end()) 
            return it->second.get();

        return nullptr;
    }
}