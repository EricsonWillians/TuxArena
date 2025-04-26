// include/TuxArena/Network.h
#pragma once

#include <cstdint> // For fixed-width integers like uint16_t, uint8_t
#include <limits>  // For numeric_limits

namespace TuxArena {
namespace Network {

// --- Constants ---
constexpr uint16_t DEFAULT_PORT = 12345; // Default UDP port used if none specified
constexpr uint32_t PROTOCOL_ID = 0x54555841; // "TUXA" - Simple protocol identifier
constexpr uint16_t PROTOCOL_VERSION = 1;     // Current protocol version
constexpr int MAX_PACKET_SIZE = 1400;        // Max bytes for UDP payload (conservative)
constexpr int MAX_CLIENTS = 8;               // Max clients supported by server (example)
constexpr float CONNECTION_TIMEOUT = 10.0f;  // Seconds before considering a client disconnected
constexpr float CLIENT_HEARTBEAT_RATE = 1.0f; // Seconds between client sending keep-alive/input

// --- Message Types ---
// The first byte of every packet identifies its type
enum class MessageType : uint8_t {
    // --- Client -> Server ---
    CONNECT_REQUEST = 1, // Client wants to connect [ProtocolID, Version, PlayerName?]
    DISCONNECT = 2,      // Client gracefully disconnecting
    INPUT = 3,           // Client sending input state [SequenceNr, InputState]
    PONG = 4,            // Client reply to server PING
    ACK = 5,             // Client acknowledging reliable message receipt

    // --- Server -> Client ---
    WELCOME = 101,       // Server acknowledges connection [ClientID, MapName, ServerInfo?]
    REJECT = 102,        // Server rejects connection [ReasonCode]
    STATE_UPDATE = 103,  // Server broadcasting game state snapshot [Timestamp, EntityStates...]
    SPAWN_ENTITY = 104,  // Server commands client to spawn an entity [EntityID, Type, Pos, Vel...]
    DESTROY_ENTITY = 105,// Server commands client to destroy an entity [EntityID]
    PING = 106,          // Server checking client connection
    RELIABLE_MSG = 107,  // Wrapper for messages requiring acknowledgement
    SET_MAP = 108,       // Command client to load a specific map

    // Add more as needed (chat, specific game events, etc.)
};

// --- Basic Data Structures (Examples - Refine as needed) ---

// Example: Structure for player input sent from client to server
// Needs proper serialization/deserialization
struct PlayerInputState {
    uint32_t sequenceNumber; // To handle out-of-order/dropped packets
    // Input flags (use bitmask or individual bools)
    bool moveForward : 1;
    bool moveBackward : 1;
    bool moveLeft : 1;
    bool moveRight : 1;
    bool shoot : 1;
    // Add other boolean actions...
    uint8_t reserved : 3; // Padding bits
    // Aiming direction (send periodically or when changed significantly)
    float aimAngleDegrees; // Or send aim vector components (aimX, aimY)
    // Add weapon switch commands, etc.

    // TODO: Implement serialize/deserialize methods for network transport
};

// Example: Reason for connection rejection
enum class RejectReason : uint8_t {
    SERVER_FULL,
    INVALID_VERSION,
    INVALID_PROTOCOL,
    BANNED,
    // Add others
};


// --- Helper Functions (Optional) ---

// inline void serialize_uint32(uint8_t* buffer, uint32_t value) { ... }
// inline uint32_t deserialize_uint32(const uint8_t* buffer) { ... }
// Implement basic network byte order conversions (htonl, ntohl equivalent) if needed


} // namespace Network
} // namespace TuxArena