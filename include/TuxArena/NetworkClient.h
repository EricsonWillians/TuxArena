// include/TuxArena/NetworkClient.h
#pragma once

#include "TuxArena/Network.h" // Common network definitions
#include "SDL3_net/SDL_net.h"
#include <cstdint>
#include <string>
#include <vector> // For potential state buffering

namespace TuxArena {

// Forward declarations
class EntityManager;
class InputManager;
class MapManager;
class ModManager;

// Connection state for the client
enum class ConnectionState {
    DISCONNECTED,
    RESOLVING_HOST,
    SENDING_REQUEST,
    CONNECTING,       // Waiting for WELCOME/REJECT
    CONNECTED,
    CONNECTION_FAILED,
    DISCONNECTING     // Optional: If graceful disconnect has stages
};


class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    // Prevent copying
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    /**
     * @brief Initializes the client socket and prepares for connection attempts.
     * @param entityManager Pointer to the entity manager for state application.
     * @param inputManager Pointer to the input manager for sending input.
     * @param mapManager Pointer to the map manager (e.g., to load map specified by server).
     * @param modManager Pointer to the mod manager.
     * @return True on success, false otherwise.
     */
    bool initialize(EntityManager* entityManager,
                    InputManager* inputManager,
                    MapManager* mapManager,
                    ModManager* modManager);

    /**
     * @brief Shuts down the client, closes the socket, and frees resources.
     */
    void shutdown();

    /**
     * @brief Attempts to connect to the specified server. Asynchronous operation.
     * @param serverIp The IP address or hostname of the server.
     * @param serverPort The UDP port of the server.
     * @param playerName The name the player wishes to use.
     * @return True if the connection attempt process was initiated, false on immediate failure (e.g., resolve error).
     */
    bool connect(const std::string& serverIp, uint16_t serverPort, const std::string& playerName);

    /**
     * @brief Disconnects from the server (if connected). Sends a disconnect message.
     */
    void disconnect();

    /**
     * @brief Gathers input and sends it to the server (if connected). Should be called regularly.
     */
    void sendInput();

    /**
     * @brief Receives and processes incoming packets from the server. Should be called frequently.
     */
    void receiveData();

    /**
     * @brief Placeholder for client-side logic updates (e.g., prediction/interpolation adjustments).
     * Could be called within receiveData/handleStateUpdate or separately.
     */
    void update(); // TODO: Define role clearly - interpolation/prediction logic?

    // --- Status Queries ---
    bool isConnected() const;
    ConnectionState getConnectionState() const { return m_connectionState; }
    std::string getStatusString() const; // Get human-readable status


private:
    // --- Packet Handling ---
    void handlePacket(UDPpacket* packet);
    void handleWelcome(UDPpacket* packet);
    void handleReject(UDPpacket* packet);
    void handleStateUpdate(UDPpacket* packet);
    void handleSpawnEntity(UDPpacket* packet);
    void handleDestroyEntity(UDPpacket* packet);
    void handlePing(UDPpacket* packet);
    void handleSetMap(UDPpacket* packet);
    // Add handlers for other message types as needed

    /**
     * @brief Applies received server state to the local EntityManager.
     * Handles interpolation, prediction correction, etc.
     * @param buffer Pointer to the state data within the received packet.
     * @param length Length of the state data.
     * @param serverTimestamp Timestamp from the server for this state.
     */
    void applyStateSnapshot(const uint8_t* buffer, int length, double serverTimestamp);

    /**
     * @brief Sends raw data to the server address stored in m_serverAddress.
     * @param data Pointer to the data buffer.
     * @param len Length of the data in bytes.
     * @return True if send succeeded (or was queued), false otherwise.
     */
    bool sendPacketToServer(const uint8_t* data, int len);

    // --- Client State ---
    bool m_isInitialized = false;
    ConnectionState m_connectionState = ConnectionState::DISCONNECTED;
    UDPsocket m_clientSocket = nullptr;  // Client's UDP socket
    IPaddress m_serverAddress;         // Resolved address of the server
    uint32_t m_clientId = 0;           // Assigned by the server upon connection
    std::string m_playerName = "TuxClient";
    double m_lastServerPacketTime = 0.0; // Time last packet received from server
    double m_connectionAttemptTime = 0.0; // Time connection attempt started

    // Pointers to game systems (dependency injection)
    EntityManager* m_entityManager = nullptr;
    InputManager* m_inputManager = nullptr;
    MapManager* m_mapManager = nullptr;
    ModManager* m_modManager = nullptr;

    // Packet buffers
    UDPpacket* m_recvPacket = nullptr; // For receiving data
    UDPpacket* m_sendPacket = nullptr; // For sending data (reuse structure)

    // Buffer for building outgoing packets if needed separately from m_sendPacket->data
    // uint8_t m_sendBuffer[Network::MAX_PACKET_SIZE];

    // State for input sequence numbering
    uint32_t m_inputSequenceNumber = 0;

    // TODO: Add structures for snapshot buffering and interpolation
};

} // namespace TuxArena