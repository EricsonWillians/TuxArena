// src/NetworkClient.cpp
#include "TuxArena/NetworkClient.h"
#include "TuxArena/Network.h"
#include "TuxArena/EntityManager.h"
#include "TuxArena/InputManager.h"
#include "TuxArena/MapManager.h"
#include "TuxArena/ModManager.h"
#include "TuxArena/Entity.h" // For EntityContext

#include "SDL3/SDL.h" // For logging, timing
#include "SDL3_net/SDL_net.h"

#include <iostream>
#include <cstring> // For memcpy, memset
#include <stdexcept> // For potential exceptions

// --- Basic Logging Helper ---
namespace Log {
    static void Info(const std::string& msg) { std::cout << "[INFO] [Client] " << msg << std::endl; }
    static void Warning(const std::string& msg) { std::cerr << "[WARN] [Client] " << msg << std::endl; }
    static void Error(const std::string& msg) { std::cerr << "[ERROR] [Client] " << msg << std::endl; }
}
// --- End Basic Logging Helper ---


namespace TuxArena {

NetworkClient::NetworkClient() {
    // Log::Info("NetworkClient created.");
}

NetworkClient::~NetworkClient() {
    // Log::Info("NetworkClient destroyed.");
    if (m_isInitialized) {
        Log::Warning("NetworkClient destroyed without explicit shutdown() call.");
        shutdown();
    }
}

bool NetworkClient::initialize(EntityManager* entityManager,
                               InputManager* inputManager,
                               MapManager* mapManager,
                               ModManager* modManager)
{
    if (m_isInitialized) {
        Log::Warning("NetworkClient::initialize called multiple times.");
        return true;
    }
    Log::Info("Initializing NetworkClient...");

    m_entityManager = entityManager;
    m_inputManager = inputManager;
    m_mapManager = mapManager;
    m_modManager = modManager;

    // Validate dependencies (InputManager is critical for sending input)
    if (!m_entityManager || !m_inputManager || !m_mapManager /*|| !m_modManager*/) {
        Log::Error("Cannot initialize NetworkClient: Missing required system pointers.");
        return false;
    }

    if (SDLNet_WasInit() == 0) {
         Log::Error("SDL_net was not initialized before initializing NetworkClient.");
         return false;
    }

    // Allocate send/receive packet structures
    // We use one packet structure and point its data buffer as needed.
    m_recvPacket = SDLNet_AllocPacket(Network::MAX_PACKET_SIZE);
    m_sendPacket = SDLNet_AllocPacket(Network::MAX_PACKET_SIZE);
    if (!m_recvPacket || !m_sendPacket) {
        Log::Error("Failed to allocate UDP packets: " + std::string(SDLNet_GetError()));
        if (m_recvPacket) SDLNet_FreePacket(m_recvPacket);
        if (m_sendPacket) SDLNet_FreePacket(m_sendPacket);
        m_recvPacket = m_sendPacket = nullptr;
        return false;
    }

    // Open a UDP socket on any available port (port 0)
    m_clientSocket = SDLNet_UDP_Open(0);
    if (!m_clientSocket) {
        Log::Error("Failed to open UDP socket: " + std::string(SDLNet_GetError()));
        SDLNet_FreePacket(m_recvPacket);
        SDLNet_FreePacket(m_sendPacket);
        m_recvPacket = m_sendPacket = nullptr;
        return false;
    }
    // Optionally get the port we were assigned:
    // IPaddress* myAddr = SDLNet_UDP_GetPeerAddress(m_clientSocket, -1); // Get local address
    // if (myAddr) Log::Info("Client socket opened on port " + std::to_string(SDLNet_Read16(&myAddr->port)));


    m_connectionState = ConnectionState::DISCONNECTED;
    m_isInitialized = true;
    Log::Info("NetworkClient initialized successfully.");
    return true;
}

void NetworkClient::shutdown() {
    if (!m_isInitialized) return;
    Log::Info("Shutting down NetworkClient...");

    if (isConnected()) {
        disconnect(); // Attempt graceful disconnect if connected
    }

    if (m_clientSocket) {
        SDLNet_UDP_Close(m_clientSocket);
        m_clientSocket = nullptr;
        Log::Info("Client socket closed.");
    }

    if (m_recvPacket) { SDLNet_FreePacket(m_recvPacket); m_recvPacket = nullptr; }
    if (m_sendPacket) { SDLNet_FreePacket(m_sendPacket); m_sendPacket = nullptr; }

    m_entityManager = nullptr;
    m_inputManager = nullptr;
    m_mapManager = nullptr;
    m_modManager = nullptr;

    m_connectionState = ConnectionState::DISCONNECTED;
    m_isInitialized = false;
    Log::Info("NetworkClient shutdown complete.");
}

bool NetworkClient::connect(const std::string& serverIp, uint16_t serverPort, const std::string& playerName) {
    if (!m_isInitialized || m_connectionState != ConnectionState::DISCONNECTED) {
        Log::Warning("Connect called while not in DISCONNECTED state (" + getStatusString() + ")");
        return false;
    }

    Log::Info("Attempting to connect to " + serverIp + ":" + std::to_string(serverPort) + " as '" + playerName + "'");
    m_connectionState = ConnectionState::RESOLVING_HOST;
    m_playerName = playerName;

    // Resolve server address (this can block, consider async resolve later)
    if (SDLNet_ResolveHost(&m_serverAddress, serverIp.c_str(), serverPort) != 0) {
        Log::Error("Failed to resolve server host '" + serverIp + "': " + SDLNet_GetError());
        m_connectionState = ConnectionState::CONNECTION_FAILED;
        return false;
    }
    // Optionally print resolved IP: SDLNet_ResolveIP(&m_serverAddress) ... SDLNet_Write32

    // --- Send Connect Request ---
    m_connectionState = ConnectionState::SENDING_REQUEST;
    uint8_t buffer[Network::MAX_PACKET_SIZE]; // Temporary buffer for connect message
    memset(buffer, 0, sizeof(buffer));
    int offset = 0;

    buffer[offset++] = static_cast<uint8_t>(Network::MessageType::CONNECT_REQUEST);
    // Add Protocol ID & Version (example serialization)
    uint32_t protoIdNet = SDL_SwapBE32(Network::PROTOCOL_ID); // Use Big Endian for network standard
    uint16_t versionNet = SDL_SwapBE16(Network::PROTOCOL_VERSION);
    memcpy(buffer + offset, &protoIdNet, sizeof(protoIdNet)); offset += sizeof(protoIdNet);
    memcpy(buffer + offset, &versionNet, sizeof(versionNet)); offset += sizeof(versionNet);
    // Add Player Name
    // Ensure player name doesn't exceed buffer space minus header and null terminator
    size_t maxNameLen = sizeof(buffer) - offset - 1;
    strncpy((char*)buffer + offset, playerName.c_str(), maxNameLen);
    offset += strlen((char*)buffer + offset); // Length of name written
    buffer[offset] = '\0'; // Ensure null termination if strncpy didn't copy it
    offset++;


    if (!sendPacketToServer(buffer, offset)) {
        Log::Error("Failed to send CONNECT_REQUEST packet.");
        m_connectionState = ConnectionState::CONNECTION_FAILED;
        return false;
    }

    Log::Info("CONNECT_REQUEST sent. Waiting for server response...");
    m_connectionState = ConnectionState::CONNECTING;
    m_connectionAttemptTime = SDL_GetTicks64() / 1000.0; // Record time for timeout
    m_lastServerPacketTime = m_connectionAttemptTime; // Reset last packet time

    return true;
}

void NetworkClient::disconnect() {
    if (!m_isInitialized || m_connectionState == ConnectionState::DISCONNECTED) {
        return;
    }
    Log::Info("Disconnecting from server...");

    if (m_connectionState == ConnectionState::CONNECTED && m_clientSocket) {
        // Send disconnect message (best effort)
        uint8_t buffer[1];
        buffer[0] = static_cast<uint8_t>(Network::MessageType::DISCONNECT);
        sendPacketToServer(buffer, 1);
        Log::Info("Sent DISCONNECT message.");
    }

    // Reset state immediately
    m_connectionState = ConnectionState::DISCONNECTED;
    m_clientId = 0;
    // Optionally clear entities, reset map? Or leave that to game logic?
    // if(m_entityManager) m_entityManager->clearAllEntities(); // Example: Clear entities on disconnect

    // Note: Socket is not closed here, only in shutdown() or potentially on CONNECTION_FAILED.
    // This allows attempting reconnection without reopening the socket.
}

void NetworkClient::sendInput() {
    if (!m_isInitialized || m_connectionState != ConnectionState::CONNECTED || !m_inputManager) {
        return;
    }

    // TODO: Implement proper input state serialization into m_sendPacket->data
    // For now, send a minimal packet periodically as a keep-alive

    uint8_t* buffer = m_sendPacket->data;
    int offset = 0;
    buffer[offset++] = static_cast<uint8_t>(Network::MessageType::INPUT);

    // Add sequence number
    uint32_t seqNumNet = SDL_SwapBE32(m_inputSequenceNumber++);
    memcpy(buffer + offset, &seqNumNet, sizeof(seqNumNet)); offset += sizeof(seqNumNet);

    // Add actual input state flags and aim angle
    // Network::PlayerInputState inputState;
    // inputState.moveForward = m_inputManager->isActionPressed(GameAction::MOVE_FORWARD);
    // ... fill other fields ...
    // serializeInput(inputState, buffer + offset); offset += sizeof_serialized_input;

    // Send minimal packet for now
     m_sendPacket->len = offset;
     if (!sendPacketToServer(m_sendPacket->data, m_sendPacket->len)) {
         // Log::Warning("Failed to send input packet."); // Can be noisy
     }
}

void NetworkClient::receiveData() {
    if (!m_isInitialized || !m_clientSocket) return;

    int numReceived = SDLNet_UDP_Recv(m_clientSocket, m_recvPacket);
    while (numReceived > 0) {
        // Check if packet is from the known server address (ignore others)
        if (SDLNet_AddressEqual(&m_recvPacket->address, &m_serverAddress)) {
            m_lastServerPacketTime = SDL_GetTicks64() / 1000.0; // Update time received
            handlePacket(m_recvPacket);
        } else {
            // Log::Warning("Received packet from unexpected address.");
        }
        // Check for more packets
        numReceived = SDLNet_UDP_Recv(m_clientSocket, m_recvPacket);
    }
    if (numReceived == -1) { /* Ignore harmless errors */ }

    // --- Connection Timeout Check ---
    // If connecting or connected, check if server hasn't responded recently
    double currentTime = SDL_GetTicks64() / 1000.0;
    if ((m_connectionState == ConnectionState::CONNECTING || m_connectionState == ConnectionState::CONNECTED) &&
        (currentTime - m_lastServerPacketTime > Network::CONNECTION_TIMEOUT))
    {
         Log::Error("Connection timed out. No response from server for " + std::to_string(Network::CONNECTION_TIMEOUT) + " seconds.");
         // Need to reset state more gracefully
         if (m_connectionState == ConnectionState::CONNECTING) {
             m_connectionState = ConnectionState::CONNECTION_FAILED;
         } else {
             disconnect(); // Move to disconnected state
         }
    }
}

void NetworkClient::update() {
     if (!m_isInitialized) return;
    // This is where client-side prediction adjustments and entity state
    // interpolation logic would typically run based on received snapshots.
    // For now, leave it empty. State application happens directly in handleStateUpdate.
}


// --- Packet Handling ---

void NetworkClient::handlePacket(UDPpacket* packet) {
    if (!packet || packet->len == 0) return;
    Network::MessageType msgType = static_cast<Network::MessageType>(packet->data[0]);

    // Based on current state, decide which messages are valid
    switch (m_connectionState) {
        case ConnectionState::CONNECTING:
            if (msgType == Network::MessageType::WELCOME) handleWelcome(packet);
            else if (msgType == Network::MessageType::REJECT) handleReject(packet);
            // Ignore other messages while waiting for connect response
            break;

        case ConnectionState::CONNECTED:
            switch (msgType) {
                case Network::MessageType::STATE_UPDATE: handleStateUpdate(packet); break;
                case Network::MessageType::SPAWN_ENTITY: handleSpawnEntity(packet); break;
                case Network::MessageType::DESTROY_ENTITY: handleDestroyEntity(packet); break;
                case Network::MessageType::PING: handlePing(packet); break;
                case Network::MessageType::SET_MAP: handleSetMap(packet); break;
                // Ignore WELCOME/REJECT if already connected? Or handle as error/reset?
                case Network::MessageType::WELCOME: Log::Warning("Received WELCOME while already connected."); break;
                case Network::MessageType::REJECT: Log::Warning("Received REJECT while connected."); disconnect(); break;
                default: Log::Warning("Received unhandled message type: " + std::to_string(static_cast<int>(msgType))); break;
            }
            break;

        // Ignore packets in other states (DISCONNECTED, FAILED, etc.)
        default:
            break;
    }
}

void NetworkClient::handleWelcome(UDPpacket* packet) {
    Log::Info("Received WELCOME from server.");
    // TODO: Deserialize ClientID, MapName, etc. from packet->data + 1
    // Example: m_clientId = SDL_SwapBE32(*(uint32_t*)(packet->data + 1));
    // char mapNameBuffer[256]; strncpy(mapNameBuffer, (char*)packet->data + 1 + sizeof(uint32_t), 255); mapNameBuffer[255] = '\0';
    // std::string serverMapName = mapNameBuffer;

    uint32_t receivedClientId = 1; // Placeholder
    std::string serverMapName = "maps/arena1.tmx"; // Placeholder

    m_clientId = receivedClientId;
    m_connectionState = ConnectionState::CONNECTED;
    Log::Info("Connection established! Client ID: " + std::to_string(m_clientId) + ", Server Map: " + serverMapName);

    // Load the map specified by the server if different from current
    if (m_mapManager /* && m_mapManager->getCurrentMapName() != serverMapName */ ) {
         Log::Info("Loading map specified by server: " + serverMapName);
         // Need EntityContext for potential spawns during map load? Unlikely.
         // Need base asset path? Assume it's configured elsewhere.
         if (!m_mapManager->loadMap(serverMapName)) {
              Log::Error("Failed to load map '" + serverMapName + "' specified by server! Disconnecting.");
              disconnect();
              m_connectionState = ConnectionState::CONNECTION_FAILED; // Or different error state
         }
         // Clear existing entities when map changes? Server should send spawns.
         // if (m_entityManager) m_entityManager->clearAllEntities();
    }
}

void NetworkClient::handleReject(UDPpacket* packet) {
    // TODO: Deserialize reason code from packet->data + 1
    Network::RejectReason reason = Network::RejectReason::INVALID_PROTOCOL; // Placeholder
    Log::Error("Connection REJECTED by server. Reason: " + std::to_string(static_cast<int>(reason)));
    m_connectionState = ConnectionState::CONNECTION_FAILED;
    disconnect(); // Move to disconnected state
}

void NetworkClient::handleStateUpdate(UDPpacket* packet) {
    // Log::Info("Received STATE_UPDATE."); // Too noisy
    // TODO: Deserialize state snapshot (entities, scores, game timer etc.)
    // - Read server timestamp
    // - Read number of entities in snapshot
    // - Loop N times:
    //   - Read Entity ID, Type, Position, Velocity, Rotation, Health...
    //   - Store this data associated with the timestamp
    // Call applyStateSnapshot to update local entities (interpolation/reconciliation)
    double serverTimestamp = 0.0; // Placeholder
    applyStateSnapshot(packet->data + 1, packet->len - 1, serverTimestamp);
}

void NetworkClient::handleSpawnEntity(UDPpacket* packet) {
    Log::Info("Received SPAWN_ENTITY command.");
    // TODO: Deserialize EntityID, Type, Position, Velocity, Rotation, Size, other specific data
    // Example placeholders:
    uint32_t entityId = 0;
    EntityType type = EntityType::GENERIC;
    Vec2 pos = {0,0}, vel = {0,0}, size = {16,16};
    float rot = 0.0f;

    // Need EntityContext for creation. Use last update context? Or default?
    EntityContext spawnContext = m_currentContext; // Assuming Game updates NetworkClient's context

    Entity* newEntity = m_entityManager->createEntity(type, pos, spawnContext, rot, vel, size);
    if (newEntity) {
         // IMPORTANT: Need to override the ID assigned by local createEntity
         // with the ID sent by the server! Requires EntityManager modification maybe?
         // Or a way to create entities *with* a specific ID (server authoritative).
         Log::Warning("Need to handle server-authoritative ID assignment for spawned entity " + std::to_string(entityId));
         // Example: newEntity->forceId(entityId); // Needs method in Entity/Manager
    }
}

void NetworkClient::handleDestroyEntity(UDPpacket* packet) {
    Log::Info("Received DESTROY_ENTITY command.");
    // TODO: Deserialize EntityID
    uint32_t entityId = 0; // Placeholder
    if (m_entityManager) {
        m_entityManager->destroyEntity(entityId);
    }
}

void NetworkClient::handlePing(UDPpacket* packet) {
    // Log::Info("Received PING from server.");
    // Send PONG back immediately
    uint8_t buffer[1 + 8]; // Type + optional timestamp/payload from PING
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = static_cast<uint8_t>(Network::MessageType::PONG);
    // TODO: Copy payload from PING packet (if any) into PONG response for RTT calc
    // memcpy(buffer + 1, packet->data + 1, packet->len - 1);
    sendPacketToServer(buffer, 1 /* + packet->len - 1 */);
}

void NetworkClient::handleSetMap(UDPpacket* packet) {
    Log::Info("Received SET_MAP command.");
    // TODO: Deserialize map name
    std::string mapName = "maps/arena1.tmx"; // Placeholder
     if (m_mapManager /* && m_mapManager->getCurrentMapName() != mapName */ ) {
         Log::Info("Loading map specified by server: " + mapName);
         if (!m_mapManager->loadMap(mapName)) {
              Log::Error("Failed to load map '" + mapName + "' specified by server!");
              // Disconnect or request resend?
         }
         // Clear entities? Server should resend spawns.
         // if (m_entityManager) m_entityManager->clearAllEntities();
    }
}


void NetworkClient::applyStateSnapshot(const uint8_t* buffer, int length, double serverTimestamp) {
    // This is where the core client-side prediction / interpolation / reconciliation logic goes.
    // 1. Deserialize all entities from the buffer.
    // 2. For each entity state received:
    //    a. Find the corresponding local entity using m_entityManager->getEntityById(receivedId).
    //    b. If it's the locally controlled player:
    //       - Compare received server position with local predicted position.
    //       - If discrepancy is too large, smoothly correct local player position towards server position (reconciliation).
    //       - Re-simulate local input commands since the state timestamp to recalculate prediction base? (Advanced).
    //       - Apply authoritative state (health, score).
    //    c. If it's a remote entity:
    //       - Store the received state (pos, rot) associated with the serverTimestamp.
    //       - Interpolate the entity's render position/rotation between the last two received snapshots based on current render time. (Entity interpolation).
    //    d. If local entity doesn't exist (and shouldn't based on server state): Ignore? Or flag potential issue?
    // 3. Handle entities present locally but NOT in the snapshot (maybe destroy them if server confirms?). Requires careful state management.

    // Log::Warning("applyStateSnapshot() needs implementation (Deserialization, Interpolation, Reconciliation).");
}


bool NetworkClient::sendPacketToServer(const uint8_t* data, int len) {
     if (!m_isInitialized || m_connectionState < ConnectionState::SENDING_REQUEST || !m_clientSocket) {
         return false;
     }
     if (len <= 0 || len > Network::MAX_PACKET_SIZE) {
         Log::Warning("Attempted to send invalid size packet: " + std::to_string(len));
         return false;
     }

     // Use the pre-allocated send packet structure
     m_sendPacket->address = m_serverAddress; // Address resolved during connect()
     m_sendPacket->len = len;
     memcpy(m_sendPacket->data, data, len);

     int sent = SDLNet_UDP_Send(m_clientSocket, -1, m_sendPacket); // -1 uses packet's address
     if (sent == 0) {
         // Log::Warning("SDLNet_UDP_Send failed: " + std::string(SDLNet_GetError())); // Can be noisy
         return false;
     }
     return true;
}

// --- Status Queries ---

bool NetworkClient::isConnected() const {
    return m_isInitialized && m_connectionState == ConnectionState::CONNECTED;
}

std::string NetworkClient::getStatusString() const {
    switch (m_connectionState) {
        case ConnectionState::DISCONNECTED:     return "Disconnected";
        case ConnectionState::RESOLVING_HOST:   return "Resolving Host...";
        case ConnectionState::SENDING_REQUEST:  return "Sending Request...";
        case ConnectionState::CONNECTING:       return "Connecting...";
        case ConnectionState::CONNECTED:        return "Connected (ID: " + std::to_string(m_clientId) + ")";
        case ConnectionState::CONNECTION_FAILED:return "Connection Failed";
        case ConnectionState::DISCONNECTING:    return "Disconnecting...";
        default:                                return "Unknown State";
    }
}


} // namespace TuxArena