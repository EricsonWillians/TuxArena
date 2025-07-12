#ifndef TUXARENA_ASSETMANAGER_H
#define TUXARENA_ASSETMANAGER_H

#include <string>
#include <vector>
#include "TuxArena/ModManager.h"
#include "SDL2/SDL.h"

namespace TuxArena {

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    void initialize(ModManager* modManager);
    std::string resolvePath(const std::string& relativePath);

private:
    std::string m_basePath;
    ModManager* m_modManager = nullptr;
};

} // namespace TuxArena

#endif // TUXARENA_ASSETMANAGER_H
