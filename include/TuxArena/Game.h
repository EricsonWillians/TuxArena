// include/TuxArena/Game.h
#pragma once // Modern include guard

#include <string>
#include <memory> // For std::unique_ptr
#include <vector>
#include <SDL2/SDL_stdinc.h> // For Uint64
#include "TuxArena/Constants.h"
#include "TuxArena/Entity.h" // Include Entity.h for EntityContext definition
#include "TuxArena/CharacterManager.h"
#include "TuxArena/UIManager.h" // Include UIManager header

// Forward declarations to minimize header dependencies
// These tell the compiler these types exist without needing their full definition here.
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;

namespace TuxArena {
    class Renderer;
    class InputManager;
    class NetworkClient;
    class NetworkServer;
    class EntityManager;
    class MapManager;
    class ModManager;
    class CharacterManager;
    class UIManager; // Forward declaration for UIManager
    class AssetManager;
class WeaponManager; // Forward declaration for WeaponManager
    // struct EntityContext; // No longer needed as Entity.h is included
    // Add PhysicsEngine if it becomes a separate component
    // class PhysicsEngine;
} // namespace TuxArena


// Configuration structure passed from main
// Could be moved to a dedicated Config.h if it grows complex
struct AppConfig {
    bool isServer = false;
    std::string serverIp = "127.0.0.1";
    int serverPort = DEFAULT_SERVER_PORT;
    std::string mapName = "arena1.tmx";
    int windowWidth = DEFAULT_WINDOW_WIDTH;
    int windowHeight = DEFAULT_WINDOW_HEIGHT;
    bool vsyncEnabled = true;
    int serverMaxPlayers = MAX_PLAYERS;
    std::string playerName = "Player"; // Default player name
    std::string playerTexturePath = ""; // Path to selected character texture
    std::string playerCharacterId = ""; // ID of the selected character
};


namespace TuxArena {

    enum class GameState
    {
        INITIALIZING,
        MAIN_MENU,
        LOBBY,
        CONNECT_TO_SERVER,
        HOSTING_GAME,
        CHARACTER_SELECTION,
        LOADING,
        PLAYING,
        PAUSED,
        ERROR_STATE,
        SHUTTING_DOWN
    };

    class Game
    {
    public:
        Game();
        ~Game();

        bool init(const AppConfig& config);
        void run();
        void shutdown();

        // Game State Management
        void pushState(GameState state);
        void popState();
        GameState currentState() const;

        // Getters for UIManager
        Renderer* getRenderer() const { return m_renderer.get(); }
        CharacterManager* getCharacterManager() const { return m_characterManager.get(); }
        NetworkClient* getNetworkClient() const { return m_networkClient.get(); }
        NetworkServer* getNetworkServer() const { return m_networkServer.get(); }
        const AppConfig& getConfig() const { return m_config; }
        const std::vector<std::string>& getAvailableMaps() const { return m_availableMaps; }
        int& getSelectedMapIndex() { return m_selectedMapIndex; }
        char* getServerIpBuffer() { return m_serverIpBuffer; }

    private:
        AppConfig m_config;
        std::vector<GameState> m_gameStateStack;
        bool m_isInitialized = false;
        bool m_isRunning = false;
    GameState m_gameState;

        std::unique_ptr<Renderer> m_renderer;
        std::unique_ptr<InputManager> m_inputManager;
        std::unique_ptr<MapManager> m_mapManager;
        std::unique_ptr<EntityManager> m_entityManager;
        std::unique_ptr<NetworkClient> m_networkClient;
        std::unique_ptr<NetworkServer> m_networkServer;
        std::unique_ptr<ModManager> m_modManager;
        std::unique_ptr<ParticleManager> m_particleManager;
        std::unique_ptr<CharacterManager> m_characterManager;
        std::unique_ptr<WeaponManager> m_weaponManager;
    std::unique_ptr<AssetManager> m_assetManager;
        std::unique_ptr<UIManager> m_uiManager; // New UIManager member


    // --- Timing ---
    Uint64 m_lastFrameTime = 0;     // Performance counter timestamp of the previous frame
    Uint64 m_perfFrequency = 0;     // Performance counter frequency
    double m_connectionAttemptTime = 0.0; // Time when client connection was initiated

    // Entity Context (passed to entities during update)
    EntityContext m_currentContext; // Added missing member

    // Private helper methods
    void handleInput();
    void update(double deltaTime);
    void render();
    void renderNonPlayingState();
    void networkUpdateReceive(double currentTime);
    void networkUpdateSend(double currentTime, double& lastSendTime);
    void updateGameState();
    void populateEntityContext(EntityContext& context, double deltaTime);
    void cleanupNetworkResources();

    // Game Actions
    void hostGame();
    void connectToServer();
    void findAvailableMaps();

    // UI State
    char m_serverIpBuffer[256];
    std::vector<std::string> m_availableMaps;
    int m_selectedMapIndex = 0;
    std::string m_playerCharacterId; // Store selected character ID

};

} // namespace TuxArena