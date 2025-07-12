#include "TuxArena/AssetManager.h"
#include "TuxArena/Log.h"
#include <filesystem>

namespace TuxArena {

AssetManager::AssetManager() {
    // Log::Info("AssetManager created.");
}

AssetManager::~AssetManager() {
    // Log::Info("AssetManager destroyed.");
}

void AssetManager::initialize(ModManager* modManager) {
    m_modManager = modManager;
    char* basePath = SDL_GetBasePath();
    if (basePath) {
        m_basePath = basePath;
        SDL_free(basePath);
    } else {
        Log::Warning("Could not get base path. Asset loading may fail.");
        m_basePath = "./";
    }
}

std::string AssetManager::resolvePath(const std::string& relativePath) {
    if (m_modManager) {
        std::string modPath = m_modManager->resolvePath(relativePath);
        if (!modPath.empty()) {
            return modPath;
        }
    }

    std::string fullPath = m_basePath + relativePath;
    if (std::filesystem::exists(fullPath)) {
        return fullPath;
    }

    return ""; // Not found
}

} // namespace TuxArena
