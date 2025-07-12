#ifndef TUXARENA_CHARACTERMANAGER_H
#define TUXARENA_CHARACTERMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory> // For std::unique_ptr
#include <SDL2/SDL.h> // For SDL_Texture

namespace TuxArena
{
    class Renderer; // Forward declaration

    #include "TuxArena/CharacterInfo.h" // Include the new CharacterInfo struct

    struct CharacterInfo;

class CharacterManager {
    {
    public:
        CharacterManager();
        ~CharacterManager();

        bool initialize(Renderer* renderer, ModManager* modManager);
        void shutdown();

        const std::vector<CharacterInfo>& getAvailableCharacters() const { return m_availableCharacters; }
        const CharacterInfo* getSelectedCharacter() const;
        void selectCharacter(const std::string& characterId);
        bool isCharacterSelected() const { return m_selectedCharacterIndex != -1; }
        std::string getSelectedCharacterId() const { return getSelectedCharacter() ? getSelectedCharacter()->id : ""; }

        // For UI rendering
        void renderSelectionUI(Renderer& renderer, int windowWidth, int windowHeight);
        void handleInput(SDL_Event& event);
        bool startGameRequested() const { return m_startGameRequested; }
        void resetStartGameRequest() { m_startGameRequested = false; }

        // Modding API
        void addCharacterDefinition(const CharacterInfo& charInfo);

    private:
        Renderer* m_renderer = nullptr;
        ModManager* m_modManager = nullptr; // Added ModManager pointer
        std::vector<CharacterInfo> m_availableCharacters;
        std::map<std::string, size_t> m_characterIdToIndex; // Map ID to index in vector
        int m_selectedCharacterIndex = -1;
        bool m_startGameRequested = false;

        void loadCharactersFromModManager(); // New function to load from ModManager
    };

} // namespace TuxArena

#endif // TUXARENA_CHARACTERMANAGER_H
