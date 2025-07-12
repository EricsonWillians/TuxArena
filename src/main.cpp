// src/main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::stoi exception
#include <algorithm> // For std::find

#include "SDL2/SDL.h"
#include "SDL2/SDL_main.h" // Ensures cross-platform entry point consistency
#include "SDL2/SDL_net.h" // For SDLNet_Init, SDLNet_Quit, SDLNet_GetError
#include "TuxArena/Game.h"
#include "TuxArena/Log.h"

// Helper function to parse command-line arguments
AppConfig parseArguments(int argc, char* argv[]) {
    AppConfig config;
    std::vector<std::string> args(argv + 1, argv + argc);
    bool connectArgUsed = false;

    TuxArena::Log::Info("TuxArena Starting...");

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--server") {
            config.isServer = true;
            connectArgUsed = true; // Prevent client mode from being auto-selected
        } else if (args[i] == "--connect" && i + 1 < args.size()) {
            config.isServer = false; // Explicitly client if connecting
            config.serverIp = args[++i];
            connectArgUsed = true;
        } else if (args[i] == "--port" && i + 1 < args.size()) {
            try {
                config.serverPort = std::stoi(args[++i]);
                if (config.serverPort <= 0 || config.serverPort > 65535) {
                     TuxArena::Log::Warning("Invalid port number '" + args[i] + "'. Must be between 1 and 65535. Using default " + std::to_string(config.serverPort));
                     // Revert to default or handle appropriately
                     // config.serverPort = 12345; // Reverting to default example
                }
            } catch (const std::invalid_argument& e) {
                TuxArena::Log::Warning("Invalid port number format '" + args[i] + "'. Using default " + std::to_string(config.serverPort));
            } catch (const std::out_of_range& e) {
                 TuxArena::Log::Warning("Port number '" + args[i] + "' out of range. Using default " + std::to_string(config.serverPort));
            }
        } else if (args[i] == "--map" && i + 1 < args.size()) {
            config.mapName = args[++i];
        } else if (args[i] == "--width" && i + 1 < args.size()) {
             try {
                 config.windowWidth = std::stoi(args[++i]);
             } catch (...) { /* Handle error */ }
        } else if (args[i] == "--height" && i + 1 < args.size()) {
             try {
                 config.windowHeight = std::stoi(args[++i]);
             } catch (...) { /* Handle error */ }
        }
        else if (args[i] == "--help" || args[i] == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --server         Run as dedicated server.\n";
            std::cout << "  --connect <ip>   Connect to server at <ip> (default: 127.0.0.1 if not server).\n";
            std::cout << "  --port <port>    Server/Client port (default: " << config.serverPort << ").\n";
            std::cout << "  --map <mapfile>  Map to load (server) or expect (client) (default: " << config.mapName << ").\n";
            std::cout << "  --width <px>     Window width (client only, default: " << config.windowWidth << ").\n";
            std::cout << "  --height <px>    Window height (client only, default: " << config.windowHeight << ").\n";
            std::cout << "  --help, -h       Show this help message.\n";
            exit(0); // Exit after showing help
        } else {
            TuxArena::Log::Warning("Unknown argument '" + args[i] + "'");
        }
    }

    // Default behavior if no specific mode is set
    if (!config.isServer && !connectArgUsed) {
        config.isServer = false; // Default to client mode
        config.serverIp = "127.0.0.1"; // Default connection target
        TuxArena::Log::Info("No mode specified, defaulting to client connecting to " + config.serverIp + ":" + std::to_string(config.serverPort));
    } else if (config.isServer) {
        TuxArena::Log::Info("Mode: Server | Port: " + std::to_string(config.serverPort) + " | Map: " + config.mapName);
    } else { // Client mode specified via --connect
        TuxArena::Log::Info("Mode: Client | Connecting to: " + config.serverIp + ":" + std::to_string(config.serverPort));
    }

    return config;
}

// Standard C++ main function signature
int main(int argc, char* argv[]) {
    // 1. Parse Command Line Arguments
    AppConfig config = parseArguments(argc, argv);

    // 2. Initialize SDL Core Systems (Video, Audio, Events, Gamepad)
    //    Networking (SDL_net) might be initialized later within the Network module.
    Uint32 sdl_init_flags = SDL_INIT_EVENTS;
    if (!config.isServer) {
        // Only initialize video, audio, gamepad for the client
        sdl_init_flags |= SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK;
    } else {
        // Server might only need timer/events
        // SDL_INIT_TIMER is deprecated in SDL3, use SDL_INIT_EVENTS for basic event loop
        // or SDL_INIT_NOPARACHUTE for timer functionality if needed without other subsystems.
        // For a server, SDL_INIT_EVENTS is usually sufficient.
    }

    if (SDL_Init(sdl_init_flags) < 0) {
        TuxArena::Log::Error("FATAL ERROR: SDL could not initialize! SDL_Error: " + std::string(SDL_GetError()));
        return 1;
    }
    TuxArena::Log::Info("SDL Initialized successfully.");

    // Initialize SDL_net globally (required before using any SDL_net functions)
    // It's safe to call even if only client or server uses it.
    if (SDLNet_Init() < 0) {
         TuxArena::Log::Error("FATAL ERROR: SDL_net could not initialize! SDL_GetError: " + std::string(SDL_GetError()));
         SDL_Quit();
         return 1;
    }
    TuxArena::Log::Info("SDL_net Initialized successfully.");


    // 3. Create and Initialize Game Instance
    TuxArena::Game game;
    if (!game.init(config)) {
        TuxArena::Log::Error("FATAL ERROR: Failed to initialize game!");
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // 4. Run the Game Loop
    try {
        game.run();
    } catch (const std::exception& e) {
        TuxArena::Log::Error("FATAL ERROR: Unhandled exception during game loop: " + std::string(e.what()));
        // Perform emergency shutdown if possible
        game.shutdown();
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    } catch (...) {
         TuxArena::Log::Error("FATAL ERROR: Unknown unhandled exception during game loop.");
         game.shutdown();
         SDLNet_Quit();
         SDL_Quit();
         return 1;
    }


    // 5. Cleanup
    TuxArena::Log::Info("Initiating shutdown...");
    game.shutdown(); // Game object handles its own component cleanup

    SDLNet_Quit(); // Shutdown SDL_net
    SDL_Quit();    // Shutdown SDL subsystems
    TuxArena::Log::Info("SDL and subsystems shut down.");
    TuxArena::Log::Info("TuxArena exited gracefully.");

    return 0;
}