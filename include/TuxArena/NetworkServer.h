// include/TuxArena/NetworkServer.h
#pragma once

#include "TuxArena/Network.h" // Common network definitions
#include "SDL3_net/SDL_net.h" // SDL_net types (IPaddress, UDPsocket, UDPpacket)
#include <cstdint>
#include <vector>
#include <string>
#include <map> // For managing clients

namespace TuxArena {

// Forward declarations
class EntityManager;
class MapManager;
class ModManager;

// Structure to hold information about a connected client
struct ClientInfo {
    uint32_t clientId = 0; // Unique ID assigned by the server (distinct from Entity ID)
    IPaddress address;     // Client's UDP address and port
    std::string playerName;
    double lastPacketTime = 0.0; // Time the last packet was received (for timeouts)
    uint32_t lastInputSequence = 0; // Last processed input sequence number
    bool isConnected = false; // Flag indicating active connection after handshake
    // Add state relevant for reliable messaging, latency tracking etc.
    // Entity* playerEntity = nullptr; // Pointer to the player entity controlled by this client
};


class NetworkServer {
public:
    NetworkServer();
    ~NetworkServer();

    // Prevent copying
    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;

    /**
     * @brief Initializes the server socket and prepares for connections.
     * @param port The UDP port number to listen on.
     * @param maxClients Maximum number of clients allowed.
     * @param entityManager Pointer to the entity manager for state access.
     * @param mapManager Pointer to the map manager.
     * @param modManager Pointer to the mod manager.
     * @return True on success, false otherwise.
     */
    bool initialize(uint16_t port,
                    int maxClients,
                    EntityManager* entityManager,
                    MapManager* mapManager,
                    ModManager* modManager);

    /**
     * @brief Shuts down the server, closes the socket, and disconnects clients.
     */
    void shutdown();

    /**
     * @brief Receives and processes incoming packets from clients. Should be called frequently.
     */
    void receiveData();

    /**
     * @brief Sends necessary state updates and other messages to connected clients.
     * Should be called at a regular interval (e.g., server tick rate).
     */
    void sendUpdates();

    /**
     * @brief Checks for and disconnects clients that have timed out.
     * @param currentTime The current game time.
     */
    void checkTimeouts(double currentTime);

private:
    /**
     * @brief Sends raw data to a specific client address.
     * @param dest The destination IP address and port.
     * @param data Pointer to the data buffer.
     * @param len Length of the data in bytes.
     * @return True if send succeeded (or was queued), false otherwise.
     */
    bool sendPacket(const IPaddress& dest, const uint8_t* data, int len);

    /**
     * @brief Sends raw data to all connected clients, optionally excluding one.
     * @param data Pointer to the data buffer.
     * @param len Length of the data in bytes.
     * @param excludeClientAddress Optional address of a client to skip.
     */
    void broadcastPacket(const uint8_t* data, int len, const IPaddress* excludeClientAddress = nullptr);


    // --- Packet Handling ---
    /**
     * @brief Processes a received UDP packet.
     * @param packet The received packet.
     */
    void handlePacket(UDPpacket* packet);

    /**
     * @brief Handles a connection request from a potential new client.
     * @param packet The connection request packet.
     * @param address The source address of the request.
     */
    void handleConnectRequest(UDPpacket* packet, const IPaddress& address);

    /**
     * @brief Handles an input packet from a connected client.
     * @param packet The input packet.
     * @param client The client information associated with the source address.
     */
    void handleClientInput(UDPpacket* packet, ClientInfo& client);

     /**
      * @brief Handles a disconnect message from a client.
      * @param packet The disconnect packet.
      * @param client The client information associated with the source address.
      */
    void handleClientDisconnect(UDPpacket* packet, ClientInfo& client);

    /**
     * @brief Handles a Pong message from a client.
     * @param packet The pong packet.
     * @param client The client information.
     */
    void handleClientPong(UDPpacket* packet, ClientInfo& client);


    // --- State Management ---
    /**
     * @brief Serializes the current relevant game state into a buffer for network transmission.
     * @param buffer The buffer to write serialized data into.
     * @param bufferSize The maximum size of the buffer.
     * @return The number of bytes written to the buffer.
     */
    int serializeGameState(uint8_t* buffer, int bufferSize);

    /**
     * @brief Adds a new client or finds an existing one based on address.
     * @param address The client's address.
     * @return Pointer to the ClientInfo struct, or nullptr if server is full or allocation fails.
     */
    ClientInfo* findOrAddClient(const IPaddress& address);

    /**
     * @brief Removes a client from the server's management.
     * @param clientId The unique ID of the client to remove.
     */
    void removeClient(uint32_t clientId);

    // --- Server State ---
    bool m_isInitialized = false;
    UDPsocket m_serverSocket = nullptr; // Server's listening socket
    uint16_t m_port = 0;
    int m_maxClients = 0;
    uint32_t m_nextClientId = 1; // Counter for assigning client IDs

    // Client Management
    std::map<uint32_t, ClientInfo> m_clients; // Map ClientID -> ClientInfo
    // Need efficient lookup from IPAddress to ClientID as well for packet handling
    // SDLNet_AddressEqual can compare IPaddress structs
    struct AddressHasher { size_t operator()(const IPaddress& addr) const; };
    struct AddressEqual { bool operator()(const IPaddress& a, const IPaddress& b) const; };
    std::unordered_map<IPaddress, uint32_t, AddressHasher, AddressEqual> m_addressToClientIdMap;


    // Pointers to game systems (dependency injection)
    EntityManager* m_entityManager = nullptr;
    MapManager* m_mapManager = nullptr;
    ModManager* m_modManager = nullptr;

    // Packet buffer for receiving data
    UDPpacket* m_recvPacket = nullptr;

    // Buffer for building outgoing packets
    uint8_t m_sendBuffer[Network::MAX_PACKET_SIZE];
};

} // namespace TuxArena