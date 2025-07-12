// src/NetworkServer.cpp
#include "TuxArena/NetworkServer.h"
#include "SDL2/SDL_net.h"
#include "TuxArena/Constants.h"
#include "TuxArena/EntityManager.h"
#include "TuxArena/MapManager.h"
#include "TuxArena/ModManager.h"
#include "TuxArena/EntityManager.h"
#include "TuxArena/MapManager.h"
#include "TuxArena/ModManager.h"
#include "SDL2/SDL_net.h"
#include "TuxArena/Network.h" // Common definitions
#include "TuxArena/EntityManager.h"
#include "TuxArena/MapManager.h"
#include "TuxArena/ModManager.h"
#include "TuxArena/Log.h" // For basic logging

// Potentially include specific entity headers if needed for state serialization

#include "SDL2/SDL.h" // For logging, timing (SDL_GetTicks64, etc.)

#include <vector>
#include <cstring> // For memcpy, memset

namespace TuxArena {

// --- Hash and Equal functors for using IPaddress in unordered_map ---
size_t NetworkServer::AddressHasher::operator()(const IPaddress& addr) const {
    // Simple hash based on host and port. Improve if needed.
    // Ensure host is in network byte order before hashing if consistency across machines matters,
    // but for map key on one machine, direct value is usually fine.
    size_t h1 = std::hash<uint32_t>{}(addr.host);
    size_t h2 = std::hash<uint16_t>{}(addr.port);
    return h1 ^ (h2 << 1); // Combine hashes
}

bool NetworkServer::AddressEqual::operator()(const IPaddress& a, const IPaddress& b) const {
    // Use SDLNet_AddressEqual for reliable comparison
    return (a.host == b.host && a.port == b.port);
}
// --- End Hash and Equal functors ---


NetworkServer::NetworkServer() {
    // Log::Info("NetworkServer created.");
}

NetworkServer::~NetworkServer() {
    // Log::Info("NetworkServer destroyed.");
    if (m_isInitialized) {
        Log::Warning("NetworkServer destroyed without explicit shutdown() call.");
        shutdown();
    }
}

bool NetworkServer::initialize(int port, int maxClients,
                    EntityManager* entityManager, MapManager* mapManager)
{
    Log::Info("NetworkServer::initialize() called with port: " + std::to_string(port) + ", maxClients: " + std::to_string(maxClients));
    if (m_isInitialized) {
        Log::Warning("NetworkServer::initialize called multiple times.");
        return true;
    }
    Log::Info("Initializing NetworkServer on port " + std::to_string(port) + "...");

    m_port = port;
    m_maxClients = maxClients > 0 ? maxClients : MAX_PLAYERS;
    m_entityManager = entityManager;
    m_mapManager = mapManager;

    // Validate dependencies
    if (!m_entityManager || !m_mapManager /*|| !m_modManager*/) {
        Log::Error("Cannot initialize NetworkServer: Missing required system pointers (EntityManager, MapManager).");
        return false;
    }

    // SDLNet_Init() should have been called in main.cpp
    if (SDLNet_Init() == -1) {
         Log::Error("SDL_net could not be initialized: " + std::string(SDLNet_GetError()));
         return false;
    }

    // Open the server socket on the specified port
    m_serverSocket = SDLNet_UDP_Open(m_port);
    if (!m_serverSocket) {
        Log::Error("Failed to open UDP socket: " + std::string(SDLNet_GetError()));
        return false;
    }
    Log::Info("Server socket opened successfully on port " + std::to_string(m_port));

    // Allocate a packet buffer for receiving data
    m_recvPacket = SDLNet_AllocPacket(Network::MAX_PACKET_SIZE);
    if (!m_recvPacket) {
        Log::Error("Failed to allocate receive packet: " + std::string(SDLNet_GetError()));
        SDLNet_UDP_Close(m_serverSocket);
        m_serverSocket = nullptr;
        return false;
    }

    m_clients.clear();
    m_addressToClientIdMap.clear();
    m_nextClientId = 1;
    m_isInitialized = true;
    Log::Info("NetworkServer initialized successfully. Max clients: " + std::to_string(m_maxClients));
    return true;
}

void NetworkServer::shutdown() {
    if (!m_isInitialized) {
        return;
    }
    Log::Info("Shutting down NetworkServer...");

    // Notify connected clients (optional, send disconnect message)
    // TODO: Send disconnect messages

    // Close the server socket
    if (m_serverSocket) {
        SDLNet_UDP_Close(m_serverSocket);
        m_serverSocket = nullptr;
        Log::Info("Server socket closed.");
    }

    // Free allocated packet(s)
    if (m_recvPacket) {
        SDLNet_FreePacket(m_recvPacket);
        m_recvPacket = nullptr;
    }

    // Clear client data
    m_clients.clear();
    m_addressToClientIdMap.clear();

    // Release system pointers (don't delete, just release ownership/reference)
    m_entityManager = nullptr;
    m_mapManager = nullptr;
    

    m_isInitialized = false;
    Log::Info("NetworkServer shutdown complete.");
}

void NetworkServer::receiveData() {
    if (!m_isInitialized || !m_serverSocket) return;

    // Try to receive a packet (non-blocking check)
    int numReceived = SDLNet_UDP_Recv(m_serverSocket, m_recvPacket);

    while (numReceived > 0) {
        // Packet received, process it
        handlePacket(m_recvPacket);

        // Check for more packets immediately available
        numReceived = SDLNet_UDP_Recv(m_serverSocket, m_recvPacket);
    }

    // numReceived == 0 means no packet available
    // numReceived == -1 means error
    if (SDLNet_UDP_Recv(m_serverSocket, m_recvPacket) == -1) {
        // Don't log constantly if it's just "No packets available" type errors
        // Log real errors if necessary. For now, assume non-critical.
        // Log::Warning("NET_UDP_Recv error: " + std::string(SDL_GetError()));
    }
}

void NetworkServer::handlePacket(UDPpacket* packet) {
    if (!packet || packet->len == 0) return; // Ignore empty packets

    // Get client address
    IPaddress sourceAddress = packet->address;

    // Log every packet received for debugging
    const char* ipStr = SDLNet_ResolveIP(&sourceAddress);
    Log::Info("Received packet from " + std::string(ipStr) + ":" + std::to_string(SDLNet_Read16(&sourceAddress.port)) + " with length " + std::to_string(packet->len));

    // Find client based on address
    auto mapIt = m_addressToClientIdMap.find(sourceAddress);
    ClientInfo* client = nullptr;
    if (mapIt != m_addressToClientIdMap.end()) {
         auto clientIt = m_clients.find(mapIt->second);
         if (clientIt != m_clients.end()) {
              client = &clientIt->second;
              // Update last packet time for timeout checks
              client->lastPacketTime = SDL_GetTicks() / 1000.0; // Store time in seconds
         }
    }

    // Read message type (first byte)
    Network::MessageType msgType = static_cast<Network::MessageType>(packet->data[0]);

    // Handle based on type
    if (msgType == Network::MessageType::CONNECT_REQUEST) {
        handleConnectRequest(packet, sourceAddress);
    } else if (client && client->isConnected) { // Only process other messages if client is known and connected
        switch (msgType) {
            case Network::MessageType::INPUT:
                handleClientInput(packet, *client);
                break;
            case Network::MessageType::DISCONNECT:
                handleClientDisconnect(packet, *client);
                break;
            case Network::MessageType::PONG:
                 handleClientPong(packet, *client);
                 break;
            // Handle ACK, other client->server messages
            default:
                Log::Warning("Received unknown or unexpected message type (" + std::to_string(static_cast<int>(msgType)) + ") from client ID " + std::to_string(client->clientId));
                break;
        }
    } else if (!client && msgType != Network::MessageType::CONNECT_REQUEST) {
         Log::Warning("Received message type " + std::to_string(static_cast<int>(msgType)) + " from unknown or unconnected address.");
         // Optionally send a REJECT or ignore.
    }
    // If client exists but is not connected (e.g., handshake not complete), might ignore messages other than handshake replies.
}


void NetworkServer::handleConnectRequest(UDPpacket* packet, const IPaddress& address) {
    // Basic validation: MessageType + ProtocolID + Version + at least 1 char for name + null terminator
    if (packet->len < 1 + sizeof(uint32_t) + sizeof(uint16_t) + 2) {
        Log::Warning("Received invalid CONNECT_REQUEST (too short).");
        return;
    }

    int offset = 1; // Start after message type

    // 1. Validate Protocol ID and Version
    uint32_t protoId;
    memcpy(&protoId, packet->data + offset, sizeof(uint32_t));
    protoId = SDL_SwapBE32(protoId); // Convert from Big Endian
    offset += sizeof(uint32_t);

    uint16_t version;
    memcpy(&version, packet->data + offset, sizeof(uint16_t));
    version = SDL_SwapBE16(version); // Convert from Big Endian
    offset += sizeof(uint16_t);

    if (protoId != Network::PROTOCOL_ID || version != Network::PROTOCOL_VERSION) {
        Log::Warning("Connection rejected: Invalid protocol or version.");
        // TODO: Send REJECT (INVALID_PROTOCOL / WRONG_VERSION) message
        return;
    }

    // 2. Check if server is full
    if (m_clients.size() >= static_cast<size_t>(m_maxClients)) {
        Log::Warning("Connection rejected: Server full (" + std::to_string(m_clients.size()) + "/" + std::to_string(m_maxClients) + ").");
        // TODO: Send REJECT (SERVER_FULL) message
        return;
    }

    // 3. Extract Player Name
    char playerNameBuffer[256];
    // Calculate remaining length for the name
    int nameLen = packet->len - offset;
    if (nameLen > 255) nameLen = 255; // Prevent buffer overflow
    strncpy(playerNameBuffer, (char*)packet->data + offset, nameLen);
    playerNameBuffer[nameLen] = '\0'; // Ensure null termination
    std::string playerName = playerNameBuffer;


    // 4. Add or find client entry
    ClientInfo* client = findOrAddClient(address);
    if (!client) {
        Log::Error("Failed to add client entry for new connection.");
        return; // Could not allocate client info
    }

    // 5. Finalize Client Setup
    client->playerName = playerName;
    client->isConnected = true;
    client->lastPacketTime = SDL_GetTicks() / 1000.0;

    Log::Info("Client connected: ID=" + std::to_string(client->clientId) + ", Name=" + client->playerName);

    // 6. Send Welcome Message
    // Buffer: [MessageType::WELCOME, ClientID]
    uint8_t welcomeBuffer[1 + sizeof(uint32_t)];
    welcomeBuffer[0] = static_cast<uint8_t>(Network::MessageType::WELCOME);
    uint32_t clientIdNet = SDL_SwapBE32(client->clientId);
    memcpy(welcomeBuffer + 1, &clientIdNet, sizeof(uint32_t));
    sendPacket(address, welcomeBuffer, sizeof(welcomeBuffer));
    Log::Info("Sent WELCOME to client ID " + std::to_string(client->clientId));

    // 7. Send Set Map Message
    std::string mapName = m_mapManager ? m_mapManager->getMapName() : "default.tmx";
    size_t mapPacketLen = 1 + mapName.length() + 1;
    uint8_t mapBuffer[mapPacketLen];
    mapBuffer[0] = static_cast<uint8_t>(Network::MessageType::SET_MAP);
    strcpy((char*)mapBuffer + 1, mapName.c_str());
    sendPacket(address, mapBuffer, mapPacketLen);
    Log::Info("Sent SET_MAP to client ID " + std::to_string(client->clientId) + " with map: " + mapName);

    // 8. Spawn Player Entity
    EntityContext playerSpawnContext;
    playerSpawnContext.entityManager = m_entityManager;
    playerSpawnContext.mapManager = m_mapManager;
    playerSpawnContext.isServer = true;

    // TODO: Get spawn point from map
    Vec2 spawnPos = {100.0f, 100.0f};
    Entity* player = m_entityManager->createEntity(EntityType::PLAYER, spawnPos, playerSpawnContext);
    if (player) {
        client->playerEntity = player;
        Log::Info("Spawned player entity for client ID " + std::to_string(client->clientId) + ", Entity ID: " + std::to_string(player->getId()));

        // TODO: Send SPAWN_ENTITY messages for all *other* existing entities to the new client
        // TODO: Send SPAWN_ENTITY message for the *new* player to all *other* clients

        // Send a SPAWN_ENTITY message for the new player to the new client
        uint8_t spawnBuffer[1 + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(Vec2) + sizeof(float)];
        spawnBuffer[0] = static_cast<uint8_t>(Network::MessageType::SPAWN_ENTITY);
        uint32_t entityIdNet = SDL_SwapBE32(player->getId());
        uint8_t entityType = static_cast<uint8_t>(player->getType());
        memcpy(spawnBuffer + 1, &entityIdNet, sizeof(uint32_t));
        memcpy(spawnBuffer + 1 + sizeof(uint32_t), &entityType, sizeof(uint8_t));
        memcpy(spawnBuffer + 1 + sizeof(uint32_t) + sizeof(uint8_t), &spawnPos, sizeof(Vec2));
        float rotation = player->getRotation();
        memcpy(spawnBuffer + 1 + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(Vec2), &rotation, sizeof(float));
        sendPacket(address, spawnBuffer, sizeof(spawnBuffer));

    } else {
        Log::Error("Failed to spawn player entity for client ID " + std::to_string(client->clientId));
    }
}

void NetworkServer::handleClientInput(UDPpacket* packet, ClientInfo& client) {
     // TODO: Deserialize Network::PlayerInputState from packet->data + 1
     // Validate input sequence number to handle out-of-order/duplicate packets
     // Apply input to the corresponding player entity controlled by this client
     // Example: if (client.playerEntity) { client.playerEntity->applyNetworkInput(deserializedInput); }
     // Log::Info("Received INPUT from client ID " + std::to_string(client.clientId));
}

void NetworkServer::handleClientDisconnect(UDPpacket* packet, ClientInfo& client) {
    (void)packet; // Suppress unused parameter warning
    (void)client; // Suppress unused parameter warning
    Log::Info("Client ID " + std::to_string(client.clientId) + " (" + client.playerName + ") sent DISCONNECT message.");
    // TODO: Properly handle graceful disconnect
    // Destroy associated player entity?
    // if (client.playerEntity && m_entityManager) { m_entityManager->destroyEntity(client.playerEntity->getId()); }
    removeClient(client.clientId);
}

void NetworkServer::handleClientPong(UDPpacket* packet, ClientInfo& client) {
    (void)packet; // Suppress unused parameter warning
    (void)client; // Suppress unused parameter warning
    // TODO: Use pong messages to calculate round-trip time (RTT) / latency
    // The pong should contain data sent in the original ping to match them up.
    // Log::Info("Received PONG from client ID " + std::to_string(client.clientId));
}


void NetworkServer::sendUpdates() {
    if (!m_isInitialized) return;

    // Always log the current client count
    Log::Info("NetworkServer::sendUpdates() called. m_clients.size(): " + std::to_string(m_clients.size()));

    // --- Serialize Game State ---
    // Buffer: [MessageType::STATE_UPDATE, Timestamp, NumEntities, [Entity1Data], [Entity2Data], ...]
    memset(m_sendBuffer, 0, Network::MAX_PACKET_SIZE);
    m_sendBuffer[0] = static_cast<uint8_t>(Network::MessageType::STATE_UPDATE);

    // Add timestamp
    uint64_t timestamp = SDL_GetTicks64();
    memcpy(m_sendBuffer + 1, &timestamp, sizeof(uint64_t));
    int bytesWritten = 1 + sizeof(uint64_t);

    // Placeholder for number of entities (will fill this in later)
    uint8_t numEntities = 0;
    int numEntitiesOffset = bytesWritten; // Store offset to write actual count later
    bytesWritten += sizeof(uint8_t);

    if (m_entityManager) {
        for (Entity* entity : m_entityManager->getActiveEntities()) {
            // Check if there's enough space for another entity (ID, Type, PosX, PosY, Rot)
            // Assuming: uint32_t ID, uint8_t Type, float PosX, float PosY, float Rot
            const int ENTITY_DATA_SIZE = sizeof(uint32_t) + sizeof(uint8_t) + (3 * sizeof(float));
            if (bytesWritten + ENTITY_DATA_SIZE > Network::MAX_PACKET_SIZE) {
                Log::Warning("NetworkServer::sendUpdates: Packet full, cannot add more entities.");
                break;
            }

            // Serialize Entity ID
            uint32_t entityId = entity->getId();
            memcpy(m_sendBuffer + bytesWritten, &entityId, sizeof(uint32_t));
            bytesWritten += sizeof(uint32_t);

            // Serialize Entity Type
            uint8_t entityType = static_cast<uint8_t>(entity->getType());
            memcpy(m_sendBuffer + bytesWritten, &entityType, sizeof(uint8_t));
            bytesWritten += sizeof(uint8_t);

            // Serialize Position (float x, y)
            Vec2 pos = entity->getPosition();
            memcpy(m_sendBuffer + bytesWritten, &pos.x, sizeof(float));
            bytesWritten += sizeof(float);
            memcpy(m_sendBuffer + bytesWritten, &pos.y, sizeof(float));
            bytesWritten += sizeof(float);

            // Serialize Rotation (float)
            float rotation = entity->getRotation();
            memcpy(m_sendBuffer + bytesWritten, &rotation, sizeof(float));
            bytesWritten += sizeof(float);

            numEntities++;
        }
    }

    // Write the actual number of entities into the packet
    memcpy(m_sendBuffer + numEntitiesOffset, &numEntities, sizeof(uint8_t));

    // --- Broadcast State ---
    if (!m_clients.empty()) { // Only send if there are clients to send to
        broadcastPacket(m_sendBuffer, bytesWritten);
        // Log::Info("Server sent STATE_UPDATE with " + std::to_string(numEntities) + " entities, size: " + std::to_string(bytesWritten) + " bytes.");
    }
} // End of sendUpdates()

    // TODO: Send reliable messages (spawn/destroy events) separately with ACK handling
    // TODO: Send periodic PING messages

void NetworkServer::checkTimeouts(double currentTime) {
    if (!m_isInitialized) return;

    std::vector<uint32_t> timedOutClients;
    for (auto const& [clientId, clientInfo] : m_clients) {
        if (clientInfo.isConnected && (currentTime - clientInfo.lastPacketTime > Network::CONNECTION_TIMEOUT)) {
            timedOutClients.push_back(clientId);
            Log::Warning("Client ID " + std::to_string(clientId) + " (" + clientInfo.playerName + ") timed out.");
        }
    }

    for (uint32_t clientId : timedOutClients) {
         // Destroy associated player entity?
         // if (m_clients.count(clientId) && m_clients[clientId].playerEntity && m_entityManager) {
         //     m_entityManager->destroyEntity(m_clients[clientId].playerEntity->getId());
         // }
        removeClient(clientId);
    }
}

// --- Send Helpers ---

bool NetworkServer::sendPacket(const IPaddress& dest, const uint8_t* data, int len) {
    if (!m_isInitialized || !m_serverSocket || len <= 0 || len > Network::MAX_PACKET_SIZE) {
        return false;
    }

    UDPpacket* sendPkt = SDLNet_AllocPacket(len);
    if (!sendPkt) {
        Log::Error("Failed to allocate send packet!");
        return false;
    }

    sendPkt->address = dest;
    sendPkt->len = len;
    memcpy(sendPkt->data, data, len);

    if (SDLNet_UDP_Send(m_serverSocket, -1, sendPkt) == 0) {
        Log::Warning("SDLNet_UDP_Send failed: " + std::string(SDLNet_GetError()));
        SDLNet_FreePacket(sendPkt);
        return false;
    }

    SDLNet_FreePacket(sendPkt);
    return true;
}


void NetworkServer::broadcastPacket(const uint8_t* data, int len, const IPaddress* excludeClientAddress) {
    if (!m_isInitialized || !m_serverSocket) return;

    for (auto const& [clientId, clientInfo] : m_clients) {
        if (clientInfo.isConnected) {
             // Check if this client should be excluded
             if (excludeClientAddress && (clientInfo.address.host == excludeClientAddress->host && clientInfo.address.port == excludeClientAddress->port)) {
                 continue; // Skip excluded client
             }
             sendPacket(clientInfo.address, data, len);
        }
    }
}

// --- Client Management Helpers ---

ClientInfo* NetworkServer::findOrAddClient(const IPaddress& address) {
    // Check if address already exists
    auto mapIt = m_addressToClientIdMap.find(address);
    if (mapIt != m_addressToClientIdMap.end()) {
        // Found existing client ID, return pointer to their info
        auto clientIt = m_clients.find(mapIt->second);
        if (clientIt != m_clients.end()) {
             Log::Info("Found existing client entry for address.");
            return &clientIt->second;
        } else {
             // Map contains ID but client info doesn't? Should not happen.
             Log::Error("CRITICAL: Client address map inconsistency! Removing stale entry.");
             m_addressToClientIdMap.erase(mapIt);
             // Attempt to proceed as if it was a new client
        }
    }

    // If not found, treat as a new client (check capacity first - done in handleConnectRequest)
    uint32_t newClientId = m_nextClientId++;
    Log::Info("Attempting to add new client with ID: " + std::to_string(newClientId));

    ClientInfo newClient;
    newClient.clientId = newClientId;
    newClient.address = address;
    newClient.isConnected = false; // Not fully connected until welcome sequence complete
    newClient.lastInputSequence = 0;
    newClient.lastPacketTime = SDL_GetTicks() / 1000.0;

    // Insert into main client map
    auto insertResult = m_clients.insert({newClientId, newClient});
    if (!insertResult.second) {
         Log::Error("Failed to insert new client info for ID: " + std::to_string(newClientId));
         m_nextClientId--; // Roll back ID increment
         return nullptr;
    }

    // Insert into address lookup map
    m_addressToClientIdMap[address] = newClientId;

    Log::Info("Added new client entry with ID: " + std::to_string(newClientId));
    return &insertResult.first->second; // Return pointer to the newly inserted ClientInfo
}


void NetworkServer::removeClient(uint32_t clientId) {
     auto clientIt = m_clients.find(clientId);
     if (clientIt != m_clients.end()) {
         Log::Info("Removing client ID " + std::to_string(clientId) + " (" + clientIt->second.playerName + ")");
         // Remove from address map first
         m_addressToClientIdMap.erase(clientIt->second.address);
         // Remove from main client map
         m_clients.erase(clientIt);
     } else {
         Log::Warning("Attempted to remove non-existent client ID: " + std::to_string(clientId));
     }
}


// --- State Serialization Placeholder ---
int NetworkServer::serializeGameState(uint8_t* buffer, int bufferSize) {
    (void)buffer; // Suppress unused parameter warning
    (void)bufferSize; // Suppress unused parameter warning
    (void)buffer; // Suppress unused parameter warning
    (void)bufferSize; // Suppress unused parameter warning
    // TODO: Implement actual game state serialization
    // - Iterate through relevant entities in m_entityManager
    // - Write ID, Type, Position, Rotation, Velocity, Health, etc.
    // - Use delta compression against previous state sent to each client
    // - Use bitpacking for efficiency
    // - Handle buffer overflow checks carefully
    Log::Warning("serializeGameState() is not fully implemented.");
    return 0; // Return number of bytes written
}


} // namespace TuxArena