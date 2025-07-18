#include "TuxArena/Game.h"

// Standard Library Includes
#include <chrono>    // For potential high-resolution timing if needed beyond SDL
#include <iostream>  // Used by basic Log helper
#include <memory>    // For std::unique_ptr, std::make_unique
#include <stdexcept> // For std::runtime_error
#include <string>
#include <vector>

// Project-Specific Includes
#include "TuxArena/Constants.h"   // <-- Assume constants like TICK_RATE are defined here
#include "TuxArena/Entity.h"      // Includes EntityContext, EntityType, Vec2
#include "TuxArena/EntityManager.h"
#include "TuxArena/InputManager.h"
#include "TuxArena/Log.h"         // <-- Assume a proper logging header exists
#include "TuxArena/MapManager.h"
#include "TuxArena/ModManager.h"
#include "TuxArena/NetworkClient.h"
#include "TuxArena/NetworkServer.h"
#include "TuxArena/Player.h"      // For spawning player (though moved)
#include "TuxArena/Renderer.h"
#include "TuxArena/WeaponManager.h"

// SDL Includes
#include "SDL2/SDL.h"
#include "SDL2/SDL_timer.h" // For SDL_GetPerformanceCounter/Frequency


namespace TuxArena {

// Helper function for string representation of GameState (for logging)
std::string GameStateToString(GameState state) {
    switch (state) {
        case GameState::INITIALIZING: return "INITIALIZING";
        case GameState::MAIN_MENU: return "MAIN_MENU";
        case GameState::LOBBY: return "LOBBY";
        case GameState::CONNECT_TO_SERVER: return "CONNECT_TO_SERVER";
        case GameState::HOSTING_GAME: return "HOSTING_GAME";
        case GameState::CHARACTER_SELECTION: return "CHARACTER_SELECTION";
        case GameState::LOADING: return "LOADING";
        case GameState::PLAYING: return "PLAYING";
        case GameState::PAUSED: return "PAUSED";
        case GameState::ERROR_STATE: return "ERROR_STATE";
        case GameState::SHUTTING_DOWN: return "SHUTTING_DOWN";
        default: return "UNKNOWN_STATE";
    }
}

// --- Game Constants (Consider moving to Constants.h or AppConfig) ---
const double MAX_FRAME_TIME = 0.1; // Max delta time to prevent spiral of death (100ms)
// Server tick rate (updates per second)
const double SERVER_TICK_RATE = 60.0;
const double SERVER_FIXED_DELTA_TIME = 1.0 / SERVER_TICK_RATE;
// Network send rates (updates per second)
const double SERVER_STATE_SEND_RATE = 30.0;
const double CLIENT_INPUT_SEND_RATE = 60.0;


// --- Game Class Implementation ---

Game::Game() {
    // Initialize members that don't rely on external data
    Log::Info("Game instance created.");
}

Game::~Game() {
     // Explicit shutdown() call is preferred for controlled cleanup.
     if (m_isInitialized && m_gameState != GameState::SHUTTING_DOWN) {
         Log::Warning("Game object destroyed without calling shutdown() first or while in invalid state. Attempting cleanup...");
         // shutdown(); // Calling complex virtual/cleanup logic in dtor is risky
     }
    Log::Info("Game instance destroyed.");
}


bool Game::init(const AppConfig& config) {
    if (m_isInitialized) {
        Log::Warning("Game::init() called multiple times.");
        return true;
    }

    try {
        m_config = config;
        pushState(GameState::INITIALIZING);

        // Initialize server IP buffer from config
        strncpy(m_serverIpBuffer, m_config.serverIp.c_str(), sizeof(m_serverIpBuffer) - 1);
        m_serverIpBuffer[sizeof(m_serverIpBuffer) - 1] = '\0'; // Ensure null termination

        Log::Info("Initializing Game systems...");
        Log::Info(m_config.isServer ? "Mode: Dedicated Server" : "Mode: Client");

        // Initialize performance counter
        m_perfFrequency = SDL_GetPerformanceFrequency();
        if (m_perfFrequency == 0) {
            Log::Error("SDL High Performance Counter not available");
            m_perfFrequency = 1000; // Fallback to millisecond precision
        }

        // Initialize core systems that are common to both client and server
        Log::Info("Initializing ModManager...");
        m_modManager = std::make_unique<ModManager>();
        if (!m_modManager->initialize("mods")) {
            Log::Warning("ModManager initialization failed. Continuing without mods.");
        } else {
            m_modManager->triggerOnInit();
        }

        Log::Info("Initializing AssetManager...");
        m_assetManager = std::make_unique<AssetManager>();
        m_assetManager->initialize(m_modManager.get());

        Log::Info("Initializing MapManager...");
        m_mapManager = std::make_unique<MapManager>();

        Log::Info("Initializing EntityManager...");
        m_entityManager = std::make_unique<EntityManager>();

        // If this is a dedicated server, initialize server-specific components and we're done
        if (m_config.isServer) {
            Log::Info("Initializing NetworkServer for dedicated mode...");
            m_networkServer = std::make_unique<NetworkServer>();
            if (!m_networkServer->initialize(m_config.serverPort, m_config.serverMaxPlayers,
                                           m_entityManager.get(), m_mapManager.get())) {
                throw std::runtime_error("Dedicated NetworkServer initialization failed");
            }
            popState(); // Pop INITIALIZING
            pushState(GameState::PLAYING); // Dedicated server goes straight to playing
        }
        // Otherwise, initialize all the client-side systems
        else {
            Log::Info("Initializing Renderer...");
            m_renderer = std::make_unique<Renderer>();
            if (!m_renderer->initialize("TuxArena", m_config.windowWidth, m_config.windowHeight, m_config.vsyncEnabled)) {
                throw std::runtime_error("Renderer initialization failed");
            }

            Log::Info("Initializing ParticleManager...");
            m_particleManager = std::make_unique<ParticleManager>();

            Log::Info("Initializing InputManager...");
            m_inputManager = std::make_unique<InputManager>();

            Log::Info("Initializing CharacterManager...");
            m_characterManager = std::make_unique<CharacterManager>();
            if (!m_characterManager->initialize(m_renderer.get(), m_modManager.get())) {
                throw std::runtime_error("CharacterManager initialization failed");
            }

            Log::Info("Initializing UIManager...");
            m_uiManager = std::make_unique<UIManager>(this);
            if (!m_uiManager->initialize(m_renderer->getSDLWindow(), m_renderer->getSDLRenderer())) {
                throw std::runtime_error("UIManager initialization failed");
            }

            // Find available maps for client UI
            findAvailableMaps();
            popState(); // Pop INITIALIZING
            pushState(GameState::MAIN_MENU);
        }

        if (m_modManager) {
            m_modManager->triggerOnGameInit();
        }

        m_isInitialized = true;
        m_isRunning = true;
        m_lastFrameTime = SDL_GetPerformanceCounter();

        Log::Info("Game initialization successful. State: " + GameStateToString(currentState()));
        return true;
    }
    catch (const std::exception& e) {
        Log::Error("FATAL ERROR during Game::init(): " + std::string(e.what()));
        pushState(GameState::ERROR_STATE);
        shutdown();
        return false;
    }
    catch (...) {
        Log::Error("FATAL ERROR during Game::init(): Unknown exception");
        pushState(GameState::ERROR_STATE);
        shutdown();
        return false;
    }
}


void Game::run() {
    if (!m_isInitialized || m_gameState == GameState::ERROR_STATE) {
        Log::Error("Game::run() called in invalid state (not initialized or error).");
        m_isRunning = false; // Ensure loop doesn't run
        return;
    }
    Log::Info("Starting main game loop...");

    // --- Timing & Fixed Timestep (Server) ---
    double accumulator = 0.0; // For fixed timestep accumulation (server)
    double lastNetworkSendTime = 0.0; // Timer for network send rate control
    double currentTime = SDL_GetPerformanceCounter() / static_cast<double>(m_perfFrequency);

    // --- Main Loop ---
    while (m_isRunning) {
        // Calculate delta time for this frame
        double newTime = SDL_GetPerformanceCounter() / static_cast<double>(m_perfFrequency);
        double deltaTime = newTime - currentTime;
        currentTime = newTime;

        // Clamp delta time to prevent issues after pauses or large hitches
        if (deltaTime > MAX_FRAME_TIME) {
            Log::Warning("Delta time clamped from " + std::to_string(deltaTime) + "s to " + std::to_string(MAX_FRAME_TIME) + "s");
            deltaTime = MAX_FRAME_TIME;
        }

        // --- Update Game State Machine (Placeholder) ---
        updateGameState(); // Check for transitions (e.g., Connecting -> Playing)


        // --- Loop Phases (Driven by Game State) ---

        // 1. Input Processing (Client only, usually always polled)
        if (!m_config.isServer && m_inputManager) {
            handleInput();
            if (m_inputManager->quitRequested()) {
                m_isRunning = false;
            }
        } else if (m_config.isServer) {
            // Check for server quit signals (e.g., console command, OS signal)
             // Example: Basic SDL quit event check for dedicated server window
             SDL_Event event;
             while(SDL_PollEvent(&event)) {
                 if (event.type == SDL_QUIT) { m_isRunning = false; }
             }
        }
        // Exit loop immediately if quit requested
        if (!m_isRunning) continue;


        // 2. Network Update (Receive data, check timeouts, potentially send ACKs/Pings)
        // Run network receives frequently regardless of game state to handle connect/disconnect etc.
        networkUpdateReceive(currentTime);


        // 3. Core Update Logic (Fixed Timestep for Server, Variable for Client)
        if (m_config.isServer) {
            // --- Server Fixed Timestep Logic ---
            accumulator += deltaTime;
            while (accumulator >= SERVER_FIXED_DELTA_TIME) {
                // Update game state using fixed steps
                update(SERVER_FIXED_DELTA_TIME); // Pass fixed delta time
                accumulator -= SERVER_FIXED_DELTA_TIME;
                // Increment server tick counter if needed
            }
            // Note: Interpolation factor for rendering isn't needed on server
        } else {
            // --- Client Variable Timestep Update ---
            update(deltaTime); // Update client simulation/interpolation
        }


        // 4. Network Sending (Controlled Rate)
        networkUpdateSend(currentTime, lastNetworkSendTime);


        // 5. Mod Hooks (Update)
        if (m_modManager && m_gameState == GameState::PLAYING) { // Only update mods while playing?
            m_modManager->triggerOnUpdate(static_cast<float>(deltaTime)); // Mods might use variable dt
        }


        // 6. Rendering (Client only)
        if (!m_config.isServer && m_renderer) {
             if (m_gameState == GameState::PLAYING || m_gameState == GameState::LOADING) {
                 // Optionally pass interpolation factor for smooth rendering on client:
                 // float interpolationAlpha = static_cast<float>(accumulator / SERVER_FIXED_DELTA_TIME); // If client used fixed update too
                 render(); // Render based on current interpolated/predicted state
             } else {
                  // Render main menu, loading screen, error message etc. based on state
                  renderNonPlayingState();
             }
        }
        // Server does not render graphics
    }

    Log::Info("Exited main game loop.");
}


void Game::shutdown() {
    if (!m_isInitialized && m_gameState == GameState::SHUTTING_DOWN) { return; } // Prevent double shutdown

    Log::Info("Shutting down Game systems...");
    m_gameState = GameState::SHUTTING_DOWN;
    m_isRunning = false;

    // --- Shutdown Sequence (Reverse of Initialization Recommended) ---
    // Ensure network disconnects before entity manager clears entities that might be network-related

    // 1. Mod Shutdown Hook
    if (m_modManager) { Log::Info("Triggering ModManager OnShutdown..."); m_modManager->triggerOnShutdown(); }

    // 2. Networking
    if (m_networkClient) { Log::Info("Shutting down NetworkClient..."); m_networkClient->shutdown(); m_networkClient.reset(); }
    if (m_networkServer) { Log::Info("Shutting down NetworkServer..."); m_networkServer->shutdown(); m_networkServer.reset(); }

    // 3. Entity Manager
    if (m_entityManager) { Log::Info("Shutting down EntityManager..."); m_entityManager->shutdown(); m_entityManager.reset(); }

    // 4. Map Manager
    if (m_mapManager) { Log::Info("Shutting down MapManager..."); m_mapManager->unloadMap(); m_mapManager.reset(); }

    // 5. Input Manager
    if (m_inputManager) { Log::Info("Shutting down InputManager..."); m_inputManager.reset(); }

    // 6. Renderer
    if (m_renderer) { Log::Info("Shutting down Renderer..."); m_renderer->shutdown(); m_renderer.reset(); }

    // 7. Mod Manager (Final resource cleanup)
    if (m_modManager) { Log::Info("Shutting down ModManager..."); m_modManager->shutdown(); m_modManager.reset(); }

    m_isInitialized = false;
    Log::Info("Game systems shutdown complete.");
}

// --- Game State & Loop Logic ---

void Game::updateGameState() {
     // Placeholder for state transitions
     if (!m_config.isServer && m_networkClient) {
          ConnectionState netState = m_networkClient->getConnectionState();

          if (m_gameState == GameState::LOADING && netState == ConnectionState::CONNECTED) {
               Log::Info("Network client connected, transitioning game state to PLAYING.");
               m_gameState = GameState::PLAYING;
               // Server should now send SPAWN message for player
          } else if ((m_gameState == GameState::PLAYING || m_gameState == GameState::LOADING) &&
                     (netState == ConnectionState::DISCONNECTED || netState == ConnectionState::CONNECTION_FAILED)) {
               Log::Warning("Network client disconnected or failed, transitioning game state to ERROR_STATE.");
               m_gameState = GameState::ERROR_STATE; // Or back to Main Menu state
          }
     }
     // Add other transitions (Pause, Menu, etc.)
}


void Game::update(double deltaTime) {
    // Populate the context for this frame
    populateEntityContext(m_currentContext, deltaTime);

    // Only update core game logic when in the playing state
    if (m_gameState == GameState::PLAYING) {
        // Update Entities (handles prediction on client, authoritative on server)
        if (m_entityManager) {
             // Pass the context, which includes deltaTime
            m_entityManager->update(m_currentContext);
        }

        // Update other game logic (rules, timers, physics outside entities?)
        // if (m_physicsEngine) { m_physicsEngine->step(deltaTime); }

    } else if (m_gameState == GameState::LOADING && !m_config.isServer) {
         // Update loading screen animations/logic? Client waits here.
    }
    // Handle updates for other states (e.g., menu logic)
}




void Game::renderNonPlayingState() {
     // Example: Render simple status messages based on state
     std::string statusText = "Initializing...";
     TuxArena::Color color = {200, 200, 200, 255}; // Grey

     switch(m_gameState) {
          case GameState::LOADING:
               statusText = "Loading / " + (m_networkClient ? m_networkClient->getStatusString() : "Connecting...");
               break;
          case GameState::ERROR_STATE:
               statusText = "Error: " + (m_networkClient ? m_networkClient->getStatusString() : "Initialization Failed");
               color = {255, 50, 50, 255}; // Red
               break;
          case GameState::PAUSED:
               statusText = "Paused";
               break;
          // Add MainMenu, etc.
          default:
               statusText = "Unknown State: " + std::to_string(static_cast<int>(m_gameState));
               break;
     }
     if (m_renderer) {
          // Center text roughly? Requires window dimensions. Assume defaults for now.
          m_renderer->drawText(statusText, 100, m_config.windowHeight / 2.0f, "assets/fonts/nokia.ttf", 24, color);
     }
}


void Game::render() {
    // Assumes Renderer exists (Client only)
    if (!m_renderer) return;

    m_renderer->clear(); // Clear background

    // --- Render Scene based on State ---
    if (m_gameState == GameState::PLAYING || m_gameState == GameState::LOADING) { // Show map/entities while loading too?
        // 1. Render Map Background
        if (m_mapManager && m_mapManager->isMapLoaded()) {
            m_renderer->renderMap(*m_mapManager, MapLayer::Background);
        }

        // 2. Render Entities
        if (m_entityManager) {
            m_entityManager->render(*m_renderer);
        }

        // 3. Render Map Foreground
        if (m_mapManager && m_mapManager->isMapLoaded()) {
             m_renderer->renderMap(*m_mapManager, MapLayer::Foreground);
        }

        // 4. Render UI / HUD (In-Game HUD)
         std::string networkStatus = "Offline";
         if (m_networkClient) networkStatus = m_networkClient->getStatusString();
         else if (m_networkServer) networkStatus = "Server Running";
         m_renderer->drawText(networkStatus, 10, 10, "assets/fonts/nokia.ttf", 16, {255, 255, 255, 255});
         // TODO: Render health, ammo, score etc.

    } else {
         // Render based on other states (handled by renderNonPlayingState)
         renderNonPlayingState();
    }

    // --- Render elements common to multiple states ---
    // Render Mods (might draw overlay UI)
    if (m_modManager) { m_modManager->triggerOnUpdate(0.0f); } // TODO: Pass proper delta time

    // Render Debug Info (if enabled)
    // if (m_debugEnabled) renderDebug();


    m_renderer->present(); // Swap buffers
}

void Game::pushState(GameState state) {
    m_gameStateStack.push_back(state);
}

void Game::popState() {
    if (!m_gameStateStack.empty()) {
        m_gameStateStack.pop_back();
    }
}

GameState Game::currentState() const {
    if (!m_gameStateStack.empty()) {
        return m_gameStateStack.back();
    }
    return GameState::SHUTTING_DOWN;
}

#include <dirent.h>

void Game::findAvailableMaps() {
    m_availableMaps.clear();
    std::string mapsPath = "maps";

    DIR* dir = opendir(mapsPath.c_str());
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            std::string fileName = ent->d_name;
            if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == ".tmx") {
                m_availableMaps.push_back(fileName);
            }
        }
        closedir(dir);

        // Sort maps alphabetically for consistent display
        std::sort(m_availableMaps.begin(), m_availableMaps.end());

        Log::Info("Found " + std::to_string(m_availableMaps.size()) + " map(s)");
    } else {
        Log::Warning("Maps directory not found: " + mapsPath);
    }
}



// --- Network Update Logic ---

void Game::networkUpdateReceive(double currentTime) {
     // Always try to receive packets to handle connects/disconnects etc.
     if (m_networkServer) {
          m_networkServer->receiveData();
          m_networkServer->checkTimeouts(currentTime);
     } else if (m_networkClient) {
          m_networkClient->receiveData();
          // Timeout check happens internally in receiveData for client currently
     }
}

void Game::networkUpdateSend(double currentTime, double& lastSendTime) {
     // Control send rate
     double sendInterval = 0.0;
     if (m_config.isServer) {
          sendInterval = 1.0 / SERVER_STATE_SEND_RATE;
     } else if (m_networkClient && m_networkClient->isConnected()) {
          sendInterval = 1.0 / CLIENT_INPUT_SEND_RATE;
     }

     if (sendInterval > 0.0 && (currentTime - lastSendTime >= sendInterval)) {
          if (m_networkServer) {
               m_networkServer->sendUpdates();
          } else if (m_networkClient && m_networkClient->isConnected()) {
               m_networkClient->sendInput();
          }
          lastSendTime = currentTime;
     }
}

// --- Other Helpers ---

void Game::handleInput() {
    if (m_inputManager) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            m_inputManager->processSDLEvent(event);
        }
    }
}

void Game::populateEntityContext(EntityContext& context, double deltaTime) {
    context.deltaTime = static_cast<float>(deltaTime);
    context.isServer = m_config.isServer;
    context.inputManager = m_inputManager.get();
    context.mapManager = m_mapManager.get();
    context.entityManager = m_entityManager.get();
    context.modManager = m_modManager.get();
    context.networkClient = m_networkClient.get();
    context.networkServer = m_networkServer.get();
    // context.physicsWorld = m_physicsEngine.get();
}


} // namespace TuxArena
