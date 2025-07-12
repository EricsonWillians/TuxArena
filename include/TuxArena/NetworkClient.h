#ifndef TUXARENA_NETWORKCLIENT_H
#define TUXARENA_NETWORKCLIENT_H

#include <string>
#include <cstdint>
#include <SDL2/SDL_net.h>

namespace TuxArena {

// Forward declarations
class EntityManager;
class InputManager;
class MapManager;
class ModManager;

enum class ConnectionState {
    DISCONNECTED,
    RESOLVING_HOST,
    SENDING_REQUEST,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED,
    DISCONNECTING
};

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool initialize(EntityManager* entityManager, MapManager* mapManager);
        void shutdown();

    bool connect(const std::string& ip, uint16_t port, const std::string& playerName, const std::string& playerTexturePath);
    void disconnect();

    void sendInput();
    void receiveData();

    void update();

    ConnectionState getConnectionState() const { return m_connectionState; }
    std::string getStatusString() const;
    bool isConnected() const;

private:
    bool m_isInitialized = false;
    ConnectionState m_connectionState = ConnectionState::DISCONNECTED;
    uint32_t m_clientId = 0;
    std::string m_playerName;

    // Pointers to game systems (not owned by NetworkClient)
    EntityManager* m_entityManager = nullptr;
        MapManager* m_mapManager = nullptr;

    // Network specific members
    UDPsocket m_clientSocket = nullptr;
    UDPpacket* m_recvPacket = nullptr;
    UDPpacket* m_sendPacket = nullptr;
    IPaddress m_serverAddress;
    double m_connectionAttemptTime = 0.0;
    double m_lastServerPacketTime = 0.0;
    uint32_t m_inputSequenceNumber = 0;

    // Private helper methods
    void handlePacket(UDPpacket* packet);
    void handleWelcome(UDPpacket* packet);
    void handleReject(UDPpacket* packet);
    void handleStateUpdate(UDPpacket* packet);
    void handleSpawnEntity(UDPpacket* packet);
    void handleDestroyEntity(UDPpacket* packet);
    void handlePing(UDPpacket* packet);
    void handleSetMap(UDPpacket* packet);
    void applyStateSnapshot(const uint8_t* buffer, int length, double serverTimestamp);
    bool sendPacketToServer(const uint8_t* data, int len);
};

} // namespace TuxArena

#endif // TUXARENA_NETWORKCLIENT_H
