#ifndef TUXARENA_CONSTANTS_H
#define TUXARENA_CONSTANTS_H

// Game Loop Constants
const double MAX_FRAME_TIME = 0.1; // Max delta time to prevent spiral of death (100ms)
const double SERVER_TICK_RATE = 60.0;
const double SERVER_FIXED_DELTA_TIME = 1.0 / SERVER_TICK_RATE;
const double SERVER_STATE_SEND_RATE = 30.0;
const double CLIENT_INPUT_SEND_RATE = 60.0;
const double CONNECTION_TIMEOUT_SECONDS = 5.0; // 5 seconds timeout for client connection

// Network Constants
const int DEFAULT_SERVER_PORT = 12345;
const int MAX_PLAYERS = 8;

// Renderer Constants
const int DEFAULT_WINDOW_WIDTH = 1024;
const int DEFAULT_WINDOW_HEIGHT = 768;

// Projectile Constants
const float BULLET_WIDTH = 8.0f;
const float BULLET_HEIGHT = 8.0f;

// Asset Paths
const std::string ASSETS_DIR = "/home/ericsonwillians/workspace/TuxArena/assets/";
const std::string MAPS_DIR = "/home/ericsonwillians/workspace/TuxArena/maps/";

#endif // TUXARENA_CONSTANTS_H