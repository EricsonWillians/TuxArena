#include "TuxArena/Weapon.h"
#include "TuxArena/Player.h"
#include "TuxArena/EntityManager.h"
#include "TuxArena/ProjectileBullet.h"
#include "TuxArena/Log.h"
#include <cmath> // For std::cos, std::sin

namespace TuxArena
{

    Weapon::Weapon(const WeaponDef& def, Player* owner)
        : m_definition(def), m_owner(owner), m_shootTimer(0.0f)
    {
    }

    void Weapon::update(const EntityContext& context)
    {
        if (m_shootTimer > 0)
        {
            m_shootTimer -= context.deltaTime;
        }
    }

    bool Weapon::shoot(const EntityContext& context)
    {
        if (isOnCooldown() || !m_owner) return false;

        Log::Info("Weapon firing: " + m_definition.name);

        float ownerRotation = m_owner->getRotation(); // in degrees

        for (int i = 0; i < m_definition.projectilesPerShot; ++i)
        {
            float spread = 0.0f;
            if (m_definition.spreadAngle > 0)
            {
                spread = ((float)rand() / RAND_MAX - 0.5f) * m_definition.spreadAngle;
            }

            float angle = ownerRotation + spread;

            // Calculate spawn position
            float spawnX = m_owner->getPosition().x + std::cos(angle * M_PI / 180.0f) * (m_owner->getSize().x / 2.0f + 5.0f);
            float spawnY = m_owner->getPosition().y + std::sin(angle * M_PI / 180.0f) * (m_owner->getSize().y / 2.0f + 5.0f);

            auto bullet = std::make_unique<ProjectileBullet>(context.entityManager, context.particleManager, spawnX, spawnY, angle, m_definition.projectileSpeed, m_definition.projectileDamage);
            bullet->setOwner(m_owner->getId());
            context.entityManager->addEntity(std::move(bullet));
        }

        m_shootTimer = 1.0f / m_definition.fireRate;
        return true;
    }

} // namespace TuxArena
