// src/main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::stoi exception
#include <algorithm> // For std::find

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h" // Ensures cross-platform entry point consistency
#include "TuxArena/Game.h"

// Helper function to parse command-line arguments
AppConfig parseArguments(int argc, char* argv[]) {
    AppConfig config;
    std::vector<std::string> args(argv + 1, argv + argc);
    bool connectArgUsed = false;

    std::cout << "TuxArena Starting..." << std::endl;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--server") {
            config.isServer = true;
        } else if (args[i] == "--connect" && i + 1 < args.size()) {
            config.isServer = false; // Explicitly client if connecting
            config.serverIp = args[++i];
            connectArgUsed = true;
        } else if (args[i] == "--port" && i + 1 < args.size()) {
            try {
                config.serverPort = std::stoi(args[++i]);
                if (config.serverPort <= 0 || config.serverPort > 65535) {
                     std::cerr << "Warning: Invalid port number '" << args[i] << "'. Must be between 1 and 65535. Using default " << config.serverPort << std::endl;
                     // Revert to default or handle appropriately
                     // config.serverPort = 12345; // Reverting to default example
                }
            } catch (const std::invalid_argument& e) {
                std::cerr << "Warning: Invalid port number format '" << args[i] << "'. Using default " << config.serverPort << std::endl;
            } catch (const std::out_of_range& e) {
                 std::cerr << "Warning: Port number '" << args[i] << "' out of range. Using default " << config.serverPort << std::endl;
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
            std::cerr << "Warning: Unknown argument '" << args[i] << "'" << std::endl;
        }
    }

    // Default behavior if no specific mode is set
    if (!config.isServer && !connectArgUsed) {
        config.isServer = false; // Default to client mode
        config.serverIp = "127.0.0.1"; // Default connection target
        std::cout << "No mode specified, defaulting to client connecting to " << config.serverIp << ":" << config.serverPort << std::endl;
    } else if (config.isServer) {
        std::cout << "Mode: Server | Port: " << config.serverPort << " | Map: " << config.mapName << std::endl;
    } else { // Client mode specified via --connect
        std::cout << "Mode: Client | Connecting to: " << config.serverIp << ":" << config.serverPort << std::endl;
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
        sdl_init_flags |= SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD;
    } else {
        // Server might only need timer/events
        sdl_init_flags |= SDL_INIT_TIMER; // Ensure timer subsystem is available
    }

    if (SDL_Init(sdl_init_flags) < 0) {
        std::cerr << "FATAL ERROR: SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "SDL Initialized successfully." << std::endl;

    // Initialize SDL_net globally (required before using any SDL_net functions)
    // It's safe to call even if only client or server uses it.
    if (SDLNet_Init() < 0) {
         std::cerr << "FATAL ERROR: SDLNet could not initialize! SDLNet_Error: " << SDLNet_GetError() << std::endl;
         SDL_Quit();
         return 1;
    }
     std::cout << "SDL_net Initialized successfully." << std::endl;


    // 3. Create and Initialize Game Instance
    TuxArena::Game game;
    if (!game.init(config)) {
        std::cerr << "FATAL ERROR: Failed to initialize game!" << std::endl;
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    // 4. Run the Game Loop
    try {
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: Unhandled exception during game loop: " << e.what() << std::endl;
        // Perform emergency shutdown if possible
        game.shutdown();
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    } catch (...) {
         std::cerr << "FATAL ERROR: Unknown unhandled exception during game loop." << std::endl;
         game.shutdown();
         SDLNet_Quit();
         SDL_Quit();
         return 1;
    }


    // 5. Cleanup
    std::cout << "Initiating shutdown..." << std::endl;
    game.shutdown(); // Game object handles its own component cleanup

    SDLNet_Quit(); // Shutdown SDL_net
    SDL_Quit();    // Shutdown SDL subsystems
    std::cout << "SDL and subsystems shut down." << std::endl;
    std::cout << "TuxArena exited gracefully." << std::endl;

    return 0;
}