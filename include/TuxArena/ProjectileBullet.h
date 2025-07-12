#ifndef TUXARENA_PROJECTILEBULLET_H
#define TUXARENA_PROJECTILEBULLET_H

#include "Entity.h"
#include "TuxArena/ParticleManager.h"

namespace TuxArena {

class ProjectileBullet : public Entity {
public:
    ProjectileBullet(EntityManager* manager, ParticleManager* particleManager, float x, float y, float angle, float speed, float damage);
    ~ProjectileBullet();

    void update(const EntityContext& context) override;
    void render(Renderer& renderer) override;
    void initialize(const EntityContext& context) override;

    void setOwner(uint32_t ownerId);

private:
    float m_speed;
    float m_damage;
    uint32_t m_ownerId;
    ParticleManager* m_particleManager;

    bool checkMapCollision(const Vec2& nextPos, const EntityContext& context);
    bool checkEntityCollision(const EntityContext& context);
};

} // namespace TuxArena

#endif // TUXARENA_PROJECTILEBULLET_H
