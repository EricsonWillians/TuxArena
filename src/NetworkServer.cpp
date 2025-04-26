// src/NetworkServer.cpp
#include "TuxArena/NetworkServer.h"
#include "TuxArena/Network.h" // Common definitions
#include "TuxArena/EntityManager.h"
#include "TuxArena/MapManager.h"
#include "TuxArena/ModManager.h"
// Potentially include specific entity headers if needed for state serialization

#include "SDL3/SDL.h" // For logging, timing (SDL_GetTicks64, etc.)
#include "SDL3_net/SDL_net.h"

#include <iostream> // For basic logging
#include <vector>
#include <cstring> // For memcpy, memset

// --- Basic Logging Helper ---
namespace Log {
    static void Info(const std::string& msg) { std::cout << "[INFO] [Server] " << msg << std::endl; }
    static void Warning(const std::string& msg) { std::cerr << "[WARN] [Server] " << msg << std::endl; }
    static void Error(const std::string& msg) { std::cerr << "[ERROR] [Server] " << msg << std::endl; }
}
// --- End Basic Logging Helper ---


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
    return SDLNet_AddressEqual(&a, &b) != 0;
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

bool NetworkServer::initialize(uint16_t port,
                               int maxClients,
                               EntityManager* entityManager,
                               MapManager* mapManager,
                               ModManager* modManager)
{
    if (m_isInitialized) {
        Log::Warning("NetworkServer::initialize called multiple times.");
        return true;
    }
    Log::Info("Initializing NetworkServer on port " + std::to_string(port) + "...");

    m_port = port;
    m_maxClients = maxClients > 0 ? maxClients : Network::MAX_CLIENTS;
    m_entityManager = entityManager;
    m_mapManager = mapManager;
    m_modManager = modManager;

    // Validate dependencies
    if (!m_entityManager || !m_mapManager /*|| !m_modManager*/) {
        Log::Error("Cannot initialize NetworkServer: Missing required system pointers (EntityManager, MapManager).");
        return false;
    }

    // SDLNet_Init() should have been called in main.cpp
    if (SDLNet_WasInit() == 0) {
         Log::Error("SDL_net was not initialized before initializing NetworkServer.");
         return false;
    }

    // Open the server socket on the specified port
    m_serverSocket = SDLNet_UDP_Open(m_port);
    if (!m_serverSocket) {
        Log::Error("Failed to open UDP socket on port " + std::to_string(m_port) + ": " + SDLNet_GetError());
        return false;
    }
    Log::Info("Server socket opened successfully on port " + std::to_string(m_port));

    // Allocate a packet buffer for receiving data
    m_recvPacket = SDLNet_AllocPacket(Network::MAX_PACKET_SIZE);
    if (!m_recvPacket) {
        Log::Error("Failed to allocate receive packet: " + SDLNet_GetError());
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
    m_modManager = nullptr;

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
    if (numReceived == -1) {
        // Don't log constantly if it's just "No packets available" type errors
        // Log real errors if necessary. For now, assume non-critical.
        // Log::Warning("SDLNet_UDP_Recv error: " + std::string(SDLNet_GetError()));
    }
}

void NetworkServer::handlePacket(UDPpacket* packet) {
    if (!packet || packet->len == 0) return; // Ignore empty packets

    // Get client address
    IPaddress sourceAddress = packet->address;

    // Find client based on address
    auto mapIt = m_addressToClientIdMap.find(sourceAddress);
    ClientInfo* client = nullptr;
    if (mapIt != m_addressToClientIdMap.end()) {
         auto clientIt = m_clients.find(mapIt->second);
         if (clientIt != m_clients.end()) {
              client = &clientIt->second;
              // Update last packet time for timeout checks
              client->lastPacketTime = SDL_GetTicks64() / 1000.0; // Store time in seconds
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
     Log::Info("Received CONNECT_REQUEST from new address.");
     // TODO: Basic validation (packet length, protocol ID, version)
     // Example: if (packet->len < sizeof(uint32_t) + sizeof(uint16_t)) return;
     // uint32_t protoId = deserialize_uint32(packet->data + 1);
     // uint16_t version = deserialize_uint16(packet->data + 1 + sizeof(uint32_t));
     // if (protoId != Network::PROTOCOL_ID || version != Network::PROTOCOL_VERSION) { Send REJECT; return; }

     // Check if server is full
     if (m_clients.size() >= m_maxClients) {
         Log::Warning("Connection rejected: Server full (" + std::to_string(m_clients.size()) + "/" + std::to_string(m_maxClients) + ").");
         // TODO: Send REJECT (SERVER_FULL) message
         return;
     }

     // Add or find client entry
     ClientInfo* client = findOrAddClient(address);
     if (!client) {
         Log::Error("Failed to add client entry for new connection.");
         return; // Could not allocate client info
     }

     // Assign player name (assume it's sent after version in request packet)
     // client->playerName = deserialize_string(packet->data + ...);
     client->playerName = "Player_" + std::to_string(client->clientId); // Placeholder name
     client->isConnected = true; // Mark as connected after successful handshake part
     client->lastPacketTime = SDL_GetTicks64() / 1000.0;

     Log::Info("Client connected: ID=" + std::to_string(client->clientId) + ", Name=" + client->playerName);


     // --- Send Welcome Message ---
     // Buffer: [MessageType::WELCOME, ClientID, MapName C-String]
     // Need a robust way to serialize map name and other info.
     memset(m_sendBuffer, 0, Network::MAX_PACKET_SIZE);
     m_sendBuffer[0] = static_cast<uint8_t>(Network::MessageType::WELCOME);
     // Serialize Client ID (example using memcpy - use proper serialization)
     uint32_t clientIdNet = SDL_Swap32(client->clientId); // Ensure network byte order if needed, though maybe overkill for client ID
     memcpy(m_sendBuffer + 1, &clientIdNet, sizeof(uint32_t));
     // Serialize Map Name
     std::string mapName = m_mapManager ? m_mapManager->getMapName() : "default.tmx"; // Need getMapName() in MapManager
     strncpy((char*)m_sendBuffer + 1 + sizeof(uint32_t), mapName.c_str(), Network::MAX_PACKET_SIZE - 1 - sizeof(uint32_t) - 1); // Copy with null terminator space

     int packetLen = 1 + sizeof(uint32_t) + mapName.length() + 1; // Type + ID + MapName + NullTerminator
     sendPacket(address, m_sendBuffer, packetLen);
     Log::Info("Sent WELCOME to client ID " + std::to_string(client->clientId) + " with map: " + mapName);

     // TODO: Spawn player entity for this client and send initial state / SPAWN_ENTITY message
     // Entity* player = m_entityManager->createEntity(EntityType::PLAYER, ...);
     // client->playerEntity = player; // Associate entity with client
     // Send SPAWN_ENTITY message for this player to the new client
     // Send SPAWN_ENTITY messages for all *other* existing entities to the new client
     // Send SPAWN_ENTITY message for the *new* player to all *other* clients
}

void NetworkServer::handleClientInput(UDPpacket* packet, ClientInfo& client) {
     // TODO: Deserialize Network::PlayerInputState from packet->data + 1
     // Validate input sequence number to handle out-of-order/duplicate packets
     // Apply input to the corresponding player entity controlled by this client
     // Example: if (client.playerEntity) { client.playerEntity->applyNetworkInput(deserializedInput); }
     // Log::Info("Received INPUT from client ID " + std::to_string(client.clientId));
}

void NetworkServer::handleClientDisconnect(UDPpacket* packet, ClientInfo& client) {
     Log::Info("Client ID " + std::to_string(client.clientId) + " (" + client.playerName + ") sent DISCONNECT message.");
     // TODO: Properly handle graceful disconnect
     // Destroy associated player entity?
     // if (client.playerEntity && m_entityManager) { m_entityManager->destroyEntity(client.playerEntity->getId()); }
     removeClient(client.clientId);
}

void NetworkServer::handleClientPong(UDPpacket* packet, ClientInfo& client) {
    // TODO: Use pong messages to calculate round-trip time (RTT) / latency
    // The pong should contain data sent in the original ping to match them up.
    // Log::Info("Received PONG from client ID " + std::to_string(client.clientId));
}


void NetworkServer::sendUpdates() {
    if (!m_isInitialized || m_clients.empty()) return;

    // --- Serialize Game State ---
    // This is a critical part: efficiently pack positions, health, etc.
    // for all relevant entities into the send buffer. Delta compression is key for performance.
    // For now, placeholder for simple snapshot.
    memset(m_sendBuffer, 0, Network::MAX_PACKET_SIZE);
    m_sendBuffer[0] = static_cast<uint8_t>(Network::MessageType::STATE_UPDATE);
    // Add timestamp (e.g., server frame number or high-res timer)
    // uint64_t timestamp = SDL_GetTicks64(); memcpy(m_sendBuffer + 1, &timestamp, sizeof(timestamp));
    int bytesWritten = 1; // Start after message type // + sizeof(timestamp);

    // Iterate through entities and serialize relevant ones
    // This needs access to the entity list from EntityManager
    // if (m_entityManager) {
    //     for (Entity* entity : m_entityManager->getActiveEntities()) {
    //          // Check if entity type should be synced
    //          // Serialize ID, Type, Position, Rotation, Velocity, Health etc.
    //          // Append serialized data to m_sendBuffer, update bytesWritten
    //          // Check buffer overflow!
    //          if (bytesWritten + estimatedSize > Network::MAX_PACKET_SIZE) break;
    //          // bytesWritten += serializeEntity(entity, m_sendBuffer + bytesWritten);
    //     }
    // }

    // Temporary: Send a minimal placeholder state update
     bytesWritten = 1 + sizeof(uint64_t); // Type + Placeholder timestamp
     uint64_t timeNow = SDL_GetTicks64();
     memcpy(m_sendBuffer + 1, &timeNow, sizeof(timeNow));
     m_sendBuffer[bytesWritten++] = 0; // Placeholder: Number of entities = 0

    // --- Broadcast State ---
    if (bytesWritten > 1) { // Only send if there's actual data (beyond type)
        broadcastPacket(m_sendBuffer, bytesWritten);
    }

    // TODO: Send reliable messages (spawn/destroy events) separately with ACK handling
    // TODO: Send periodic PING messages
}

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

    // Need a UDPpacket structure for sending
    // It's often efficient to reuse a single allocated packet structure
    static UDPpacket* sendPkt = SDLNet_AllocPacket(Network::MAX_PACKET_SIZE); // Static or member? Careful with threading.
    if (!sendPkt) {
        Log::Error("Failed to allocate static send packet!");
        return false; // Cannot allocate packet struct
    }

    sendPkt->address = dest;
    sendPkt->len = len;
    memcpy(sendPkt->data, data, len); // Copy data to packet buffer

    int sent = SDLNet_UDP_Send(m_serverSocket, -1, sendPkt); // -1 means use pkt->address
    // Note: SDLNet_UDP_Send returns 1 on success, 0 on failure. It doesn't queue.
    if (sent == 0) {
        Log::Warning("SDLNet_UDP_Send failed: " + std::string(SDLNet_GetError()));
        return false;
    }
    return true;
}


void NetworkServer::broadcastPacket(const uint8_t* data, int len, const IPaddress* excludeClientAddress) {
    if (!m_isInitialized || !m_serverSocket) return;

    for (auto const& [clientId, clientInfo] : m_clients) {
        if (clientInfo.isConnected) {
             // Check if this client should be excluded
             if (excludeClientAddress && SDLNet_AddressEqual(&clientInfo.address, excludeClientAddress)) {
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
    uint32_t newClientId = m_nextClientId++; // Assign next available ID
    if (newClientId == 0) { /* Handle wrap around if necessary */ m_nextClientId = 1; newClientId = m_nextClientId++; }

    ClientInfo newClient;
    newClient.clientId = newClientId;
    newClient.address = address;
    newClient.isConnected = false; // Not fully connected until welcome sequence complete
    newClient.lastInputSequence = 0;
    newClient.lastPacketTime = SDL_GetTicks64() / 1000.0;

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