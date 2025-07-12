#ifndef TUXARENA_NETWORK_H
#define TUXARENA_NETWORK_H

#include <cstdint>

namespace TuxArena {

namespace Network {

// Protocol Constants
const uint32_t PROTOCOL_ID = 0x54584101; // 'TXA' + 0x01 (TuxArena Protocol ID)
const uint16_t PROTOCOL_VERSION = 1;     // Current protocol version

// Network Configuration
const int MAX_PACKET_SIZE = 512; // Maximum size of a UDP packet in bytes
const double CONNECTION_TIMEOUT = 5.0; // Seconds before a connection is considered timed out
const double CLIENT_CONNECT_RETRY_INTERVAL = 1.0; // Seconds between connection request retries

// Message Types (from client to server and server to client)
enum class MessageType : uint8_t {
    // Connection Management
    CONNECT_REQUEST = 1,  // Client requests to connect
    WELCOME = 2,          // Server welcomes client, assigns ID
    REJECT = 3,           // Server rejects connection
    DISCONNECT = 4,       // Client or server initiates disconnect
    PING = 5,             // Used for RTT calculation and keep-alive
    PONG = 6,             // Response to PING

    // Game State Synchronization (Server to Client)
    STATE_UPDATE = 10,    // Full or partial game state snapshot
    SPAWN_ENTITY = 11,    // Server tells client to spawn an entity
    DESTROY_ENTITY = 12,   // Server tells client to destroy an entity
    SET_MAP = 13,         // Server tells client to load a specific map

    // Client Input (Client to Server)
    INPUT = 20,           // Client sends input state

    // Chat/Messaging
    CHAT_MESSAGE = 30,    // Text chat message

    // Modding/Custom Data
    MOD_DATA = 40,        // Generic message for mod-specific data

    // Error/Debug
    ERROR_MESSAGE = 99,   // Generic error message
};

// Reject Reasons
enum class RejectReason : uint8_t {
    NONE = 0,
    INVALID_PROTOCOL = 1, // Protocol ID or version mismatch
    SERVER_FULL = 2,      // No more player slots available
    BANNED = 3,           // Client is banned
    INVALID_NAME = 4,     // Player name is invalid or already taken
    UNKNOWN_ERROR = 5,
};

} // namespace Network

} // namespace TuxArena

#endif // TUXARENA_NETWORK_H
