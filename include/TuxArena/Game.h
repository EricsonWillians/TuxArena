// include/TuxArena/Game.h
#pragma once // Modern include guard

#include <string>
#include <memory> // For std::unique_ptr

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
    // Add PhysicsEngine if it becomes a separate component
    // class PhysicsEngine;
} // namespace TuxArena


// Configuration structure passed from main
// Could be moved to a dedicated Config.h if it grows complex
struct AppConfig {
    bool isServer = false;
    std::string serverIp = "127.0.0.1";
    int serverPort = 12345; // Default UDP port
    std::string mapName = "maps/arena1.tmx"; // Default map
    int windowWidth = 1024;
    int windowHeight = 768;
    std::string playerName = "Player"; // Default player name (client)
    // Add more config options as needed: vsync, max_players (server), etc.
    bool vsyncEnabled = true;
    int serverMaxPlayers = 8;
};


namespace TuxArena {

class Game {
public:
    Game();
    ~Game(); // Important for cleanup

    // Prevent copying/moving to avoid complex resource management issues
    // Especially important if components hold references to each other or the Game object.
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    Game(Game&&) = delete;
    Game& operator=(Game&&) = delete;

    /**
     * @brief Initializes all core game systems based on the configuration.
     * @param config The application configuration settings.
     * @return True if initialization was successful, false otherwise.
     */
    bool init(const AppConfig& config);

    /**
     * @brief Starts and runs the main game loop until termination.
     */
    void run();

    /**
     * @brief Shuts down all game systems and releases resources.
     */
    void shutdown();

private:
    /**
     * @brief Handles user input polling and processing (Client-side).
     */
    void handleInput();

    /**
     * @brief Updates the game state, including entities, physics, and game logic.
     * @param deltaTime Time elapsed since the last frame in seconds.
     */
    void update(double deltaTime);

    /**
     * @brief Renders the current game state to the screen (Client-side).
     */
    void render();

    /**
     * @brief Handles network communication (sending input, receiving state).
     * Called within the main loop.
     */
    void networkUpdate();

    AppConfig m_config;         // Stores the configuration used to initialize the game
    bool m_isRunning = false;   // Controls the main game loop execution
    bool m_isInitialized = false; // Tracks if init() was successful

    // --- Core Components (using smart pointers for automatic memory management) ---
    // Note: Order of declaration matters for destruction order (reverse)

    // ModManager should often be initialized early for asset overrides
    std::unique_ptr<ModManager> m_modManager;

    // Renderer and InputManager are primarily client-side
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<InputManager> m_inputManager;

    // Map and Entities are needed by both client and server (though server is authoritative)
    std::unique_ptr<MapManager> m_mapManager;
    std::unique_ptr<EntityManager> m_entityManager;

    // Physics Engine (if applicable)
    // std::unique_ptr<PhysicsEngine> m_physicsEngine;

    // Networking - only one of these will typically be active
    std::unique_ptr<NetworkClient> m_networkClient;
    std::unique_ptr<NetworkServer> m_networkServer;


    // --- Timing ---
    Uint64 m_lastFrameTime = 0;     // Performance counter timestamp of the previous frame
    Uint64 m_perfFrequency = 0;     // Performance counter frequency

};

} // namespace TuxArena