#include "TuxArena/WeaponManager.h"
#include "TuxArena/Log.h"
#include "TuxArena/ModManager.h"
#include "TuxArena/Weapon.h"

// For JSON parsing (assuming nlohmann/json is available via ModManager or directly)
#include "nlohmann/json.hpp"
#include <fstream>

namespace TuxArena {

WeaponManager::WeaponManager() {
    Log::Info("WeaponManager created.");
}

WeaponManager::~WeaponManager() {
    Log::Info("WeaponManager destroyed.");
}

bool WeaponManager::initialize(ModManager* modManager) {
    if (!modManager) {
        Log::Error("WeaponManager: ModManager is null.");
        return false;
    }
    m_modManager = modManager;
    loadWeaponDefinitions();
    Log::Info("WeaponManager initialized with " + std::to_string(m_weaponDefinitions.size()) + " weapon definitions.");
    return true;
}

void WeaponManager::shutdown() {
    m_weaponDefinitions.clear();
    m_modManager = nullptr;
    Log::Info("WeaponManager shutdown complete.");
}

const WeaponDef* WeaponManager::getWeaponDef(const std::string& weaponId) const {
    auto it = m_weaponDefinitions.find(weaponId);
    if (it != m_weaponDefinitions.end()) {
        return &it->second;
    }
    Log::Warning("Weapon definition not found for ID: " + weaponId);
    return nullptr;
}

std::unique_ptr<Weapon> WeaponManager::createWeaponInstance(const std::string& weaponId, Player* owner) {
    const WeaponDef* def = getWeaponDef(weaponId);
    if (def) {
        return std::make_unique<Weapon>(*def, owner);
    }
    return nullptr;
}

void WeaponManager::loadWeaponDefinitions() {
    // Clear existing definitions
    m_weaponDefinitions.clear();

    // Load default weapons (e.g., from a hardcoded list or default JSON)
    // For now, let's hardcode a couple of default weapons
    WeaponDef pistolDef;
    pistolDef.type = WeaponType::PISTOL;
    pistolDef.name = "Pistol";
    pistolDef.fireRate = 2.0f; // 2 shots per second
    pistolDef.projectilesPerShot = 1;
    pistolDef.projectileSpeed = 800.0f;
    pistolDef.projectileDamage = 15.0f;
    pistolDef.projectileLifetime = 1.5f;
    pistolDef.spreadAngle = 2.0f;
    pistolDef.ammoCost = 1;
    m_weaponDefinitions["pistol"] = pistolDef;

    WeaponDef shotgunDef;
    shotgunDef.type = WeaponType::SHOTGUN;
    shotgunDef.name = "Shotgun";
    shotgunDef.fireRate = 0.8f; // Less than 1 shot per second
    shotgunDef.projectilesPerShot = 8;
    shotgunDef.projectileSpeed = 700.0f;
    shotgunDef.projectileDamage = 8.0f; // Per pellet
    shotgunDef.projectileLifetime = 0.5f;
    shotgunDef.spreadAngle = 20.0f;
    shotgunDef.ammoCost = 1;
    m_weaponDefinitions["shotgun"] = shotgunDef;

    Log::Info("Loaded default weapon definitions.");

    // TODO: Integrate with ModManager to load weapon definitions from mods
    // This would involve iterating through mod directories and parsing JSON files.
    // Example (conceptual):
    // for (const auto& modPath : m_modManager->getLoadedModPaths()) {
    //     std::string weaponConfigPath = modPath + "/weapons/weapons.json";
    //     if (std::filesystem::exists(weaponConfigPath)) {
    //         std::ifstream file(weaponConfigPath);
    //         nlohmann::json jsonConfig;
    //         file >> jsonConfig;
    //         for (const auto& [id, weaponJson] : jsonConfig.items()) {
    //             WeaponDef modWeaponDef;
    //             modWeaponDef.name = weaponJson.value("name", id);
    //             modWeaponDef.fireRate = weaponJson.value("fireRate", 1.0f);
    //             // ... parse other fields ...
    //             m_weaponDefinitions[id] = modWeaponDef; // Overwrite or add
    //         }
    //     }
    // }
}

} // namespace TuxArena
