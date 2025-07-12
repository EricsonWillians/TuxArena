#ifndef TUXARENA_NETWORKSERVER_H
#define TUXARENA_NETWORKSERVER_H

#include <string>
#include <SDL2/SDL_net.h>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <functional> // For std::hash
#include <unordered_map> // For std::unordered_map
#include <SDL2/SDL_net.h> // Include SDL_net.h for UDPsocket and IPaddress definitions

namespace TuxArena {

class EntityManager;
class MapManager;

// Client information structure for the server
struct ClientInfo {
    uint32_t clientId = 0;
    IPaddress address; // Client's IP and Port
    std::string playerName;
    bool isConnected = false;
    double lastPacketTime = 0.0; // For timeout checks
    uint32_t lastInputSequence = 0;
    Entity* playerEntity = nullptr; // Pointer to the player entity controlled by this client
};

class NetworkServer {
public:
    NetworkServer();
    ~NetworkServer();

    bool initialize(int port, int maxClients,
                            EntityManager* entityManager, MapManager* mapManager);
    void shutdown();

    void receiveData();
    void sendUpdates();
    void checkTimeouts(double currentTime);

    const std::map<uint32_t, ClientInfo>& getClients() const { return m_clients; }

    private:
    bool m_isInitialized = false;
    UDPsocket m_serverSocket = nullptr;
    UDPpacket* m_recvPacket = nullptr;
    uint8_t m_sendBuffer[512]; // A buffer for sending data

    int m_port = 0;
    int m_maxClients = 0;
    uint32_t m_nextClientId = 1;

    // Pointers to game systems (not owned by NetworkServer)
    EntityManager* m_entityManager = nullptr;
    MapManager* m_mapManager = nullptr;
    

    // Custom hash and equality for IPaddress to use as map key
    struct AddressHasher {
        size_t operator()(const IPaddress& addr) const;
    };

    struct AddressEqual {
        bool operator()(const IPaddress& a, const IPaddress& b) const;
    };

    std::map<uint32_t, ClientInfo> m_clients; // Map client ID to ClientInfo
    std::unordered_map<IPaddress, uint32_t, AddressHasher, AddressEqual> m_addressToClientIdMap; // Map IPaddress to Client ID

    // Private helper methods
    void handlePacket(UDPpacket* packet);
    void handleConnectRequest(UDPpacket* packet, const IPaddress& address);
    void handleClientInput(UDPpacket* packet, ClientInfo& client);
    void handleClientDisconnect(UDPpacket* packet, ClientInfo& client);
    void handleClientPong(UDPpacket* packet, ClientInfo& client);

    bool sendPacket(const IPaddress& dest, const uint8_t* data, int len);
    void broadcastPacket(const uint8_t* data, int len, const IPaddress* excludeClientAddress = nullptr);

    ClientInfo* findOrAddClient(const IPaddress& address);
    void removeClient(uint32_t clientId);
    int serializeGameState(uint8_t* buffer, int bufferSize);
};

} // namespace TuxArena

#endif // TUXARENA_NETWORKSERVER_H
