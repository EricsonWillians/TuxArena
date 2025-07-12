#include "TuxArena/ModManager.h"
#include "TuxArena/Log.h"
#include <fstream>
#include <filesystem>
#include "nlohmann/json.hpp"

namespace TuxArena {

ModManager::ModManager() {
    Log::Info("ModManager instance created.");
}

ModManager::~ModManager() {
    Log::Info("ModManager instance destroyed.");
}

bool ModManager::initialize(const std::string& modDirectory) {
    Log::Info("ModManager initialized with directory: " + modDirectory);
    m_modDirectory = modDirectory;

    // Scan for mods and load their definitions
    if (std::filesystem::exists(m_modDirectory) && std::filesystem::is_directory(m_modDirectory)) {
        for (const auto& entry : std::filesystem::directory_iterator(m_modDirectory)) {
            if (entry.is_directory()) {
                Log::Info("Found mod directory: " + entry.path().string());
                // For now, assume each mod directory can contain weapon definitions
                loadWeaponDefinitions(entry.path().string());
                loadCharacterDefinitions(entry.path().string());
            }
        }
    } else {
        Log::Warning("Mod directory not found or is not a directory: " + m_modDirectory);
    }
    return true;
}

void ModManager::shutdown() {
    Log::Info("ModManager shutting down.");
    // TODO: Implement actual mod unloading/cleanup logic here
}

void ModManager::triggerOnInit() {
    Log::Info("ModManager: Triggering OnInit hooks.");
    // TODO: Call OnInit hooks for loaded mods
}

void ModManager::triggerOnGameInit() {
    Log::Info("ModManager: Triggering OnGameInit hooks.");
    // TODO: Call OnGameInit hooks for loaded mods
}

void ModManager::triggerOnUpdate(float deltaTime) {
    (void)deltaTime; // Suppress unused parameter warning
    // Log::Info("ModManager: Triggering OnUpdate hooks. DeltaTime: " + std::to_string(deltaTime));
    // TODO: Call OnUpdate hooks for loaded mods
}

void ModManager::triggerOnShutdown() {
    Log::Info("ModManager: Triggering OnShutdown hooks.");
    // TODO: Call OnShutdown hooks for loaded mods
}

const WeaponDef* ModManager::getWeaponDefinition(const std::string& id) const {
    auto it = m_weaponDefinitions.find(id);
    if (it != m_weaponDefinitions.end()) {
        return &it->second;
    }
    return nullptr;
}

const CharacterInfo* ModManager::getCharacterDefinition(const std::string& id) const {
    auto it = m_characterDefinitions.find(id);
    if (it != m_characterDefinitions.end()) {
        return &it->second;
    }
    return nullptr;
}

void ModManager::loadWeaponDefinitions(const std::string& modPath) {
    std::filesystem::path weaponsDir = std::filesystem::path(modPath) / "weapons";
    if (!std::filesystem::exists(weaponsDir) || !std::filesystem::is_directory(weaponsDir)) {
        Log::Info("No 'weapons' directory found in mod: " + modPath);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(weaponsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string weaponFilePath = entry.path().string();
            Log::Info("Loading weapon definition: " + weaponFilePath);
            std::ifstream file(weaponFilePath);
            if (file.is_open()) {
                try {
                    nlohmann::json j = nlohmann::json::parse(file);
                    WeaponDef def;
                    def.name = j.value("name", "Unnamed Weapon");
                    def.fireRate = j.value("fireRate", 1.0f);
                    def.projectilesPerShot = j.value("projectilesPerShot", 1);
                    def.projectileSpeed = j.value("projectileSpeed", 500.0f);
                    def.projectileDamage = j.value("projectileDamage", 10.0f);
                    def.projectileLifetime = j.value("projectileLifetime", 2.0f);
                    def.spreadAngle = j.value("spreadAngle", 0.0f);
                    def.ammoCost = j.value("ammoCost", 1);

                    // Determine WeaponType from string
                    std::string typeStr = j.value("type", "PISTOL");
                    if (typeStr == "PISTOL") def.type = WeaponType::PISTOL;
                    else if (typeStr == "SHOTGUN") def.type = WeaponType::SHOTGUN;
                    // Add more types as needed

                    m_weaponDefinitions[def.name] = def; // Use name as ID for now
                    Log::Info("  - Loaded weapon: " + def.name);
                } catch (const nlohmann::json::exception& e) {
                    Log::Error("JSON parsing error in " + weaponFilePath + ": " + e.what());
                } catch (const std::exception& e) {
                    Log::Error("Error loading weapon definition " + weaponFilePath + ": " + e.what());
                }
            } else {
                Log::Error("Could not open weapon definition file: " + weaponFilePath);
            }
        }
    }
}

void ModManager::loadCharacterDefinitions(const std::string& modPath) {
    // TODO: Implement character definition loading
}

std::string ModManager::resolvePath(const std::string& relativePath) {
    // For now, just check the base mod directory. This can be expanded to check all mods.
    std::string fullPath = m_modDirectory + "/" + relativePath;
    if (std::filesystem::exists(fullPath)) {
        return fullPath;
    }
    return "";
}
    std::filesystem::path charactersDir = std::filesystem::path(modPath) / "characters";
    if (!std::filesystem::exists(charactersDir) || !std::filesystem::is_directory(charactersDir)) {
        Log::Info("No 'characters' directory found in mod: " + modPath);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(charactersDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string characterFilePath = entry.path().string();
            Log::Info("Loading character definition: " + characterFilePath);
            std::ifstream file(characterFilePath);
            if (file.is_open()) {
                try {
                    nlohmann::json j = nlohmann::json::parse(file);
                    CharacterInfo def;
                    def.id = j.value("id", entry.path().stem().string());
                    def.name = j.value("name", def.id);
                    def.texturePath = j.value("texturePath", "");
                    def.health = j.value("health", 100.0f);
                    def.speed = j.value("speed", 200.0f);
                    def.specialAbility = j.value("specialAbility", "None");

                    m_characterDefinitions[def.id] = def; // Use ID as key
                    Log::Info("  - Loaded character: " + def.name + " (ID: " + def.id + ")");
                } catch (const nlohmann::json::exception& e) {
                    Log::Error("JSON parsing error in " + characterFilePath + ": " + e.what());
                } catch (const std::exception& e) {
                    Log::Error("Error loading character definition " + characterFilePath + ": " + e.what());
                }
            } else {
                Log::Error("Could not open character definition file: " + characterFilePath);
            }
        }
    }
}

} // namespace TuxArena
