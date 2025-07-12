#pragma once

#include <string>
#include <vector>
#include <functional> // For std::function

// Forward declarations
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;

namespace TuxArena {

enum class GameState;
class Game;

// Enum to represent UI events that can be triggered by the UIManager
enum class UIEvent {
    NONE,
    HOST_GAME_CLICKED,
    CONNECT_TO_SERVER_CLICKED,
    QUIT_CLICKED,
    BACK_TO_MAIN_MENU_CLICKED,
    START_GAME_CLICKED, // For lobby
    CHARACTER_SELECTED, // When a character is chosen
    CONNECT_ATTEMPT, // When connect button is pressed in connect screen
    RESUME_GAME_CLICKED
};

// Struct to hold data associated with a UI event
struct UIEventData {
    UIEvent type = UIEvent::NONE;
    std::string stringValue; // For map names, IP addresses, character IDs
    int intValue = 0;        // For selected indices, etc.
};

class UIManager {
public:
    UIManager(Game* game);
    ~UIManager();

    // Initialize ImGui context (should be called once at app startup)
    bool initialize(SDL_Window* window, SDL_Renderer* renderer);
    void shutdown();

    void render(GameState currentState);
    void clear();

    // Render methods for different game states
    void renderMainMenu(int windowWidth, int windowHeight);
    void renderLobby(int windowWidth, int windowHeight, const std::vector<std::string>& availableMaps, int& selectedMapIndex);
    void renderConnectToServer(int windowWidth, int windowHeight, char* serverIpBuffer, int serverPort);
    void renderCharacterSelection(int windowWidth, int windowHeight); // Placeholder for now
    void renderNonPlayingState(int windowWidth, int windowHeight, const std::string& statusText, bool showReturnToMainMenuButton);
    void renderPauseMenu(int windowWidth, int windowHeight);

    // Event handling
    void processSDLEvent(SDL_Event* event);
    UIEventData getAndClearUIEvent(); // Get the last triggered UI event and clear it

    // Setters for data UIManager needs to display
    void setNetworkStatus(const std::string& status);
    void setCharacterSelectionData(const std::vector<std::string>& characterNames, int selectedCharIndex); // Example

private:
    Game* m_game;
    SDL_Window* m_sdlWindow = nullptr;
    SDL_Renderer* m_sdlRenderer = nullptr;
    UIEventData m_lastUIEvent; // Stores the last event triggered by UI

    std::string m_networkStatus; // Status string from NetworkClient/Server
    // Character selection data (example)
    std::vector<std::string> m_characterNames;
    int m_selectedCharacterIndex = 0;

    // Helper to set common window properties
    void setupCenteredWindow(int windowWidth, int windowHeight, float widthRatio, float heightRatio);

    // Helper to set common button styling
    void pushCommonButtonStyles();
    void popCommonButtonStyles();

    // Helper to set common window styling
    void pushCommonWindowStyles();
    void popCommonWindowStyles();
};

} // namespace TuxArena
