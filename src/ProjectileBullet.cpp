// src/ProjectileBullet.cpp
#include "TuxArena/ProjectileBullet.h"
#include "TuxArena/EntityManager.h"
#include "TuxArena/MapManager.h"
#include "TuxArena/Renderer.h"
#include "TuxArena/Player.h" // Include Player to check type and apply damage

#include "SDL3/SDL.h" // For logging

#include <cmath> // For std::sqrt

namespace TuxArena {

ProjectileBullet::ProjectileBullet(EntityManager* manager) :
    Entity(manager, EntityType::PROJECTILE_BULLET) // Call base constructor
{
    // Initialize bullet-specific defaults
    setSize(6.0f, 6.0f);     // Small size for a bullet
    m_lifetime = 2.0f;       // Live for 2 seconds
    m_lifeTimer = 0.0f;
    m_damage = 10;
    m_ownerId = 0;           // Unknown owner initially
    m_isStatic = false;      // Bullets move
    // Velocity is set by the creator (e.g., Player::attemptShoot) via setVelocity()
    // std::cout << "ProjectileBullet entity created." << std::endl;
}

void ProjectileBullet::initialize(const EntityContext& context) {
    // Initialize is called right after creation by EntityManager
    // If ownerId needs to be passed reliably, it should be an argument
    // to createEntity and set here, or via a dedicated setOwner method called externally.
    // std::cout << "Bullet ID " << getId() << " initialized." << std::endl;

    // Rotation can be set based on initial velocity direction
    if (m_velocity.x != 0.0f || m_velocity.y != 0.0f) {
        m_rotation = std::atan2(m_velocity.y, m_velocity.x) * (180.0f / M_PI);
    }
}

void ProjectileBullet::setOwner(uint32_t ownerId) {
    m_ownerId = ownerId;
}

void ProjectileBullet::update(const EntityContext& context) {
    // --- Lifetime Check ---
    m_lifeTimer += context.deltaTime;
    if (m_lifeTimer >= m_lifetime) {
        // std::cout << "Bullet ID " << getId() << " lifetime expired." << std::endl;
        if (m_entityManager) { // Ensure manager pointer is valid
            m_entityManager->destroyEntity(getId());
        }
        return; // Stop further processing if destroyed
    }

    // --- Movement Calculation ---
    Vec2 deltaPos = m_velocity * context.deltaTime;
    Vec2 currentPos = getPosition();
    Vec2 nextPos = currentPos + deltaPos;

    // --- Collision Detection ---
    bool destroyed = false;

    // 1. Check Map Collision
    if (checkMapCollision(nextPos, context)) {
        destroyed = true;
        // std::cout << "Bullet ID " << getId() << " collided with map." << std::endl;
        // TODO: Spawn impact effect/sound at collision point
    }

    // 2. Check Entity Collision (only if not already destroyed by map)
    if (!destroyed) {
        if (checkEntityCollision(context)) {
            destroyed = true;
            // std::cout << "Bullet ID " << getId() << " collided with entity." << std::endl;
            // TODO: Spawn impact effect/sound
        }
    }

    // --- Final Position Update ---
    if (!destroyed) {
        setPosition(nextPos);
    } else {
        // If destroyed, queue for removal
        if (m_entityManager) {
            m_entityManager->destroyEntity(getId());
        }
    }
}


bool ProjectileBullet::checkMapCollision(const Vec2& nextPos, const EntityContext& context) {
     if (!context.mapManager) return false;

     // Simple point collision check for now (center of bullet)
     // A better check would use the bullet's bounding box
     const auto& shapes = context.mapManager->getCollisionShapes();
     for (const auto& shape : shapes) {
          // Broadphase AABB check first
          if (nextPos.x >= shape.minX && nextPos.x <= shape.maxX &&
              nextPos.y >= shape.minY && nextPos.y <= shape.maxY)
          {
                // TODO: Narrowphase - Check if point is inside polygon/rect/ellipse
                // For simplicity now, assume AABB intersection means collision for bullet
                return true;
          }
     }
     return false;
}

bool ProjectileBullet::checkEntityCollision(const EntityContext& context) {
     if (!context.entityManager) return false;

     // Find nearby entities (optimization)
     float checkRadius = m_size.x > m_size.y ? m_size.x : m_size.y; // Check radius around bullet center
     // Slightly larger radius to catch entities the bullet might move into this frame?
     // checkRadius += (velocity.length() * context.deltaTime)? - More complex calculation

     std::vector<Entity*> nearbyEntities = context.entityManager->findEntitiesInRadius(
         getPosition(),
         checkRadius * 2.0f, // Check a slightly larger radius
         [&](const Entity* entity) {
              // Filter: Don't collide with self, owner, other projectiles, or non-collidable types
              return entity &&
                     entity->isActive() &&
                     entity->getId() != getId() &&
                     entity->getId() != m_ownerId &&
                     entity->getType() != EntityType::PROJECTILE_BULLET && // Don't collide with other bullets
                     entity->getType() != EntityType::GENERIC &&
                     entity->getType() != EntityType::TRIGGER; // Example non-collidable types
         }
     );

     if (nearbyEntities.empty()) {
         return false;
     }

     // Simple AABB collision check against nearby entities
     SDL_FRect bulletRect = { getPosition().x - m_size.x / 2.0f, getPosition().y - m_size.y / 2.0f, m_size.x, m_size.y };

     for (Entity* target : nearbyEntities) {
          // Calculate target AABB (assuming getPosition is center)
          Vec2 targetPos = target->getPosition();
          Vec2 targetSize = target->getSize();
          SDL_FRect targetRect = { targetPos.x - targetSize.x / 2.0f, targetPos.y - targetSize.y / 2.0f, targetSize.x, targetSize.y };

          if (SDL_HasRectIntersectionFloat(&bulletRect, &targetRect)) {
                // Collision detected!
                // Apply damage if the target is damageable (e.g., a Player)
                if (target->getType() == EntityType::PLAYER) {
                    // Need a way to apply damage. Add a takeDamage method to Entity/Player?
                    // Example: target->takeDamage(m_damage, m_ownerId);
                    std::cout << "Bullet ID " << getId() << " hit Player ID " << target->getId() << " for " << m_damage << " damage." << std::endl;
                } else {
                    // Hit some other collidable entity type
                    std::cout << "Bullet ID " << getId() << " hit Entity ID " << target->getId() << " (Type: " << (int)target->getType() << ")" << std::endl;
                }

                // Bullet is destroyed on first hit
                return true;
          }
     }

     return false; // No collision this frame
}


void ProjectileBullet::render(Renderer& renderer) {
    // Load texture if not already loaded
    if (!m_texture) {
        m_texture = renderer.loadTexture(m_texturePath);
        if (!m_texture) {
            // Draw placeholder if texture fails
            SDL_FRect errorRect = {m_position.x - m_size.x / 2.0f, m_position.y - m_size.y / 2.0f, m_size.x, m_size.y};
            renderer.drawRect(&errorRect, {255, 255, 0, 255}); // Yellow rectangle
            return;
        }
    }

    // Destination rectangle (assuming position is center)
    SDL_FRect dstRect = {
        m_position.x - m_size.x / 2.0f,
        m_position.y - m_size.y / 2.0f,
        m_size.x,
        m_size.y
    };

    // Draw the bullet texture, potentially using m_rotation if desired
    renderer.drawTexture(m_texture, nullptr, &dstRect, m_rotation, SDL_FLIP_NONE);
}

// void ProjectileBullet::onDestroy(const EntityContext& context) {
//     // Called just before destruction
//     // Spawn particle effect, sound, etc.
//     // std::cout << "Bullet ID " << getId() << " destroyed." << std::endl;
// }


} // namespace TuxArena