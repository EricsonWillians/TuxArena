#ifndef TUXARENA_ENTITYMANAGER_H
#define TUXARENA_ENTITYMANAGER_H

#include "Entity.h"
#include "Player.h"
#include "ParticleManager.h"
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <functional> // For std::function

namespace TuxArena {

// Forward declarations
class MapManager;
class Renderer;

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    bool initialize(MapManager* mapManager);
    void shutdown();

    Entity* createEntity(EntityType type,
                         const Vec2& position,
                         const EntityContext& context,
                         float rotation = 0.0f,
                         const Vec2& velocity = {0.0f, 0.0f},
                         const Vec2& size = {16.0f, 16.0f},
                         uint32_t forceId = 0);

    void destroyEntity(uint32_t id);
    void addEntity(std::unique_ptr<Entity> entity);
    Entity* getEntityById(uint32_t id) const;

    void update(const EntityContext& context);
    void render(Renderer& renderer);
    void renderDebug(Renderer& renderer); // For debugging purposes

    std::vector<Entity*> getActiveEntities() const;
    void clearAllEntities();
    std::vector<Entity*> findEntitiesInRadius(
        const Vec2& center,
        float radius,
        const std::function<bool(const Entity*)>& filter = nullptr) const;

    Player* getPlayer(); // Assuming there's a single player for now
    ParticleManager* getParticleManager() { return &m_particleManager; }

private:
    bool m_isInitialized = false;
    MapManager* m_mapManager = nullptr;
    uint32_t m_nextEntityId = 1;

    std::vector<std::unique_ptr<Entity>> m_entities;
    std::map<uint32_t, Entity*> m_entityMap; // Raw pointers for quick lookup
    std::vector<uint32_t> m_destructionQueue;

    ParticleManager m_particleManager;

    EntityContext m_lastUpdateContext; // Store context for deferred destruction

    uint32_t assignNextId();
    void processDestructionQueue();

    Player* m_player; // Raw pointer for simplicity, assuming ownership is elsewhere or managed by m_entities
};

} // namespace TuxArena

#endif // TUXARENA_ENTITYMANAGER_H