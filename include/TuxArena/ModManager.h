#ifndef TUXARENA_MODMANAGER_H
#define TUXARENA_MODMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "TuxArena/Weapon.h" // For WeaponDef
#include "TuxArena/CharacterInfo.h" // For CharacterInfo

namespace TuxArena {

class Renderer; // Forward declaration

class ModManager {
public:
    ModManager();
    ~ModManager();

    bool initialize(const std::string& modDirectory);
    void shutdown();

    void triggerOnInit();
    void triggerOnGameInit();
    void triggerOnUpdate(float deltaTime);
    void triggerOnShutdown();

    // Modding API
    const std::map<std::string, WeaponDef>& getWeaponDefinitions() const { return m_weaponDefinitions; }
    const WeaponDef* getWeaponDefinition(const std::string& id) const;

    // Character Modding API
    const std::map<std::string, CharacterInfo>& getCharacterDefinitions() const { return m_characterDefinitions; }
    const CharacterInfo* getCharacterDefinition(const std::string& id) const;

    std::string resolvePath(const std::string& relativePath);

private:
    std::string m_modDirectory;
    std::map<std::string, WeaponDef> m_weaponDefinitions;
    std::map<std::string, CharacterInfo> m_characterDefinitions;

    void loadWeaponDefinitions(const std::string& modPath);
    void loadCharacterDefinitions(const std::string& modPath);
};

} // namespace TuxArena

#endif // TUXARENA_MODMANAGER_H