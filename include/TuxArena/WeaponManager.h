#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "TuxArena/Weapon.h"
#include "TuxArena/ModManager.h"

namespace TuxArena {

class WeaponManager {
public:
    WeaponManager();
    ~WeaponManager();

    bool initialize(ModManager* modManager);
    void shutdown();

    // Get a weapon definition by its ID
    const WeaponDef* getWeaponDef(const std::string& weaponId) const;

    // Create a new weapon instance based on a definition
    std::unique_ptr<Weapon> createWeaponInstance(const std::string& weaponId, Player* owner);

private:
    ModManager* m_modManager = nullptr;
    std::map<std::string, WeaponDef> m_weaponDefinitions; // Stores all loaded weapon definitions

    void loadWeaponDefinitions();
};

} // namespace TuxArena
