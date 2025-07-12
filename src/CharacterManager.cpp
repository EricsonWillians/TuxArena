#include "TuxArena/CharacterManager.h"
#include "TuxArena/Renderer.h"
#include "TuxArena/Log.h"
#include <filesystem> // C++17 for directory iteration
#include "nlohmann/json.hpp"
#include <fstream>
#include "TuxArena/Constants.h"

// ImGui includes
#include "imgui.h"
#include <SDL2/SDL_render.h>
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

namespace TuxArena
{

    CharacterManager::CharacterManager()
    {
        Log::Info("CharacterManager created.");
    }

    CharacterManager::~CharacterManager()
    {
        Log::Info("CharacterManager destroyed.");
    }

    bool CharacterManager::initialize(Renderer* renderer, ModManager* modManager)
    {
        if (!renderer) {
            Log::Error("CharacterManager: Renderer is null during initialization.");
            return false;
        }
        if (!modManager) {
            Log::Error("CharacterManager: ModManager is null during initialization.");
            return false;
        }
        m_renderer = renderer;
        m_modManager = modManager;

        Log::Info("CharacterManager: Loading characters from ModManager.");
        loadCharactersFromModManager();

        if (!m_availableCharacters.empty()) {
            m_selectedCharacterIndex = 0; // Select the first character by default
            Log::Info("CharacterManager: Selected initial character: " + m_availableCharacters[0].name);
        }

        Log::Info("CharacterManager initialized successfully with " + std::to_string(m_availableCharacters.size()) + " characters.");
        return true;
    }

    void CharacterManager::shutdown()
    {
        Log::Info("CharacterManager: Shutting down...");
        for (auto& charInfo : m_availableCharacters) {
            if (charInfo.texture) {
                m_renderer->destroyTexture(charInfo.texture);
                charInfo.texture = nullptr;
            }
        }
        m_availableCharacters.clear();
        m_characterIdToIndex.clear();
        m_selectedCharacterIndex = -1;
        m_renderer = nullptr;
        Log::Info("CharacterManager shutdown complete.");
    }

    const CharacterInfo* CharacterManager::getSelectedCharacter() const
    {
        if (m_selectedCharacterIndex >= 0 && static_cast<size_t>(m_selectedCharacterIndex) < m_availableCharacters.size()) {
            return &m_availableCharacters[m_selectedCharacterIndex];
        }
        return nullptr;
    }

    void CharacterManager::selectCharacter(const std::string& characterId)
    {
        auto it = m_characterIdToIndex.find(characterId);
        if (it != m_characterIdToIndex.end()) {
            m_selectedCharacterIndex = it->second;
            Log::Info("CharacterManager: Selected character: " + m_availableCharacters[m_selectedCharacterIndex].name);
        } else {
            Log::Warning("CharacterManager: Attempted to select unknown character ID: " + characterId);
        }
    }

    void CharacterManager::renderSelectionUI(Renderer& renderer, int windowWidth, int windowHeight)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.05f, 0.1f, 0.9f)); // Dark purple-ish background
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.15f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.2f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.1f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); // Light gray text

        ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(windowWidth * 0.8f, windowHeight * 0.8f), ImGuiCond_Always);
        ImGui::Begin("Character Selection", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        ImGui::Text("Choose your champion!");
        ImGui::Separator();

        float panelWidth = ImGui::GetContentRegionAvail().x;
        float buttonSize = 128.0f; // Size for character buttons
        float padding = 10.0f;
        int charactersPerRow = static_cast<int>(panelWidth / (buttonSize + padding));
        if (charactersPerRow == 0) charactersPerRow = 1;

        int i = 0;
        for (const auto& charInfo : m_availableCharacters) {
            ImGui::PushID(charInfo.id.c_str());

            // Display character image
            if (charInfo.texture) {
                ImGui::Image(static_cast<ImTextureID>(renderer.getOpenGLTextureID(charInfo.texture)), ImVec2(buttonSize, buttonSize));
            } else {
                ImGui::Dummy(ImVec2(buttonSize, buttonSize)); // Placeholder if texture not loaded
                ImGui::Text("No Image");
            }

            // Display character name
            ImGui::TextWrapped("%s", charInfo.name.c_str());

            // Selection button
            if (ImGui::RadioButton("##select", m_selectedCharacterIndex == i)) {
                m_selectedCharacterIndex = i;
                Log::Info("CharacterManager: Selected character: " + charInfo.name);
            }

            ImGui::SameLine();
            ImGui::Text(" "); // Small space

            if ((i + 1) % charactersPerRow != 0) {
                ImGui::SameLine(0.0f, padding);
            }
            ImGui::PopID();
            i++;
        }

        ImGui::Separator();
        if (ImGui::Button("Start Game")) {
            if (getSelectedCharacter()) {
                Log::Info("CharacterManager: 'Start Game' button clicked. Selected: " + getSelectedCharacter()->name);
                m_startGameRequested = true;
            } else {
                Log::Warning("CharacterManager: 'Start Game' button clicked, but no character selected.");
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(5); // Pop 5 colors
        ImGui::PopStyleVar(2); // Pop WindowRounding and WindowPadding
    }

    void CharacterManager::handleInput(SDL_Event& event)
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        // Additional input handling for character selection can go here
        // e.g., arrow keys to navigate, Enter to select
    }

    void CharacterManager::addCharacterDefinition(const CharacterInfo& charInfo) {
        // Check for duplicate IDs
        if (m_characterIdToIndex.count(charInfo.id)) {
            Log::Warning("Character with ID '" + charInfo.id + "' already exists. Overwriting.");
            // Find existing entry and update it
            size_t index = m_characterIdToIndex[charInfo.id];
            // Destroy old texture if it exists
            if (m_availableCharacters[index].texture) {
                m_renderer->destroyTexture(m_availableCharacters[index].texture);
            }
            m_availableCharacters[index] = charInfo;
        } else {
            m_availableCharacters.push_back(charInfo);
            m_characterIdToIndex[charInfo.id] = m_availableCharacters.size() - 1;
        }
    }

    void CharacterManager::loadCharactersFromModManager() {
        m_availableCharacters.clear();
        m_characterIdToIndex.clear();

        if (!m_modManager) {
            Log::Error("ModManager is null, cannot load character definitions.");
            return;
        }

        const auto& modCharacterDefs = m_modManager->getCharacterDefinitions();
        if (modCharacterDefs.empty()) {
            Log::Info("No character definitions found in ModManager. Loading default PNGs.");
            // Fallback to loading PNGs directly from assets/characters if no JSON definitions are found
            // This assumes ASSETS_DIR is defined and accessible
            try {
                for (const auto& entry : std::filesystem::directory_iterator(ASSETS_DIR + "characters")) {
                    if (entry.is_regular_file() && entry.path().extension() == ".png") {
                        std::string filePath = entry.path().string();
                        std::string fileName = entry.path().stem().string();

                        CharacterInfo charInfo;
                        charInfo.id = fileName;
                        charInfo.name = fileName;
                        charInfo.texturePath = filePath;
                        charInfo.texture = m_renderer->loadTexture(filePath);

                        if (charInfo.texture) {
                            addCharacterDefinition(charInfo);
                            Log::Info("Loaded character (PNG fallback): " + charInfo.name + " from " + filePath);
                        } else {
                            Log::Warning("Failed to load texture for character (PNG fallback): " + fileName + " from " + filePath);
                        }
                    }
                }
            } catch (const std::filesystem::filesystem_error& e) {
                Log::Error("Filesystem error loading default characters: " + std::string(e.what()));
            } catch (const std::exception& e) {
                Log::Error("Error loading default characters: " + std::string(e.what()));
            }
        } else {
            for (const auto& pair : modCharacterDefs) {
                CharacterInfo charInfo = pair.second; // Copy the definition
                // Load texture for the character
                if (!charInfo.texturePath.empty()) {
                    std::filesystem::path texPath = charInfo.texturePath;
                    if (texPath.is_relative()) {
                        charInfo.texture = m_renderer->loadTexture(ASSETS_DIR + charInfo.texturePath);
                    } else {
                        charInfo.texture = m_renderer->loadTexture(charInfo.texturePath);
                    }
                }

                if (charInfo.texture) {
                    addCharacterDefinition(charInfo);
                    Log::Info("Loaded character from mod: " + charInfo.name + " (ID: " + charInfo.id + ")");
                } else {
                    Log::Warning("Failed to load texture for modded character: " + charInfo.name + " from " + charInfo.texturePath);
                }
            }
        }
    }

} // namespace TuxArena
