// src/ProjectileBullet.cpp
#include "TuxArena/ProjectileBullet.h"
#include "TuxArena/Constants.h"
#include "TuxArena/Log.h"
#include "TuxArena/EntityManager.h" // For entity collision checks
#include "TuxArena/MapManager.h"    // For map collision checks
#include "TuxArena/Renderer.h" // For rendering

#include <cmath> // For std::sin, std::cos

namespace TuxArena {

ProjectileBullet::ProjectileBullet(EntityManager* manager, ParticleManager* particleManager, float x, float y, float angle, float speed, float damage)
    : Entity(manager, EntityType::PROJECTILE_BULLET),
      m_speed(speed),
      m_damage(damage),
      m_ownerId(0), // Default owner to 0 (no owner)
      m_particleManager(particleManager)
{
    m_position.x = x;
    m_position.y = y;
    m_size.x = BULLET_WIDTH;
    m_size.y = BULLET_HEIGHT;
    // Calculate velocity based on angle and speed
    m_velocity.x = std::cos(angle * M_PI / 180.0f) * m_speed;
    m_velocity.y = std::sin(angle * M_PI / 180.0f) * m_speed;
}

ProjectileBullet::~ProjectileBullet() {
    Log::Info("ProjectileBullet destroyed.");
}

void ProjectileBullet::initialize(const EntityContext& context) {
    // Any specific initialization for a bullet, e.g., setting its initial texture/sprite
    // For now, just call base class initialize
    Entity::initialize(context);
}

void ProjectileBullet::update(const EntityContext& context) {
    // Move the bullet
    Vec2 nextPos = m_position + m_velocity * context.deltaTime;

    // Check for map collision
    if (context.mapManager && checkMapCollision(nextPos, context)) {
        Log::Info("Bullet hit map at (" + std::to_string(nextPos.x) + ", " + std::to_string(nextPos.y) + ")");
        m_isActive = false; // Bullet is destroyed on map collision
        return;
    }

    // Check for entity collision (excluding owner)
    if (context.entityManager && checkEntityCollision(context)) {
        Log::Info("Bullet hit entity.");
        m_isActive = false; // Bullet is destroyed on entity collision
        return;
    }

    m_position = nextPos; // Update position if no collision
    if (m_particleManager) {
        m_particleManager->emitBulletTrail(m_position.x, m_position.y, m_velocity.x, m_velocity.y);
    }
    Entity::update(context); // Call base class update
}

void ProjectileBullet::render(Renderer& renderer) {
    // Render the bullet as a line
    // Calculate the bullet's rotation from its velocity
    float angle = std::atan2(m_velocity.y, m_velocity.x);
    float length = getSize().x * 1.5f; // Make the line a bit longer than the bullet's width

    // Calculate start and end points of the line, centered on the bullet's position
    float x1 = m_position.x - (length / 2.0f) * std::cos(angle);
    float y1 = m_position.y - (length / 2.0f) * std::sin(angle);
    float x2 = m_position.x + (length / 2.0f) * std::cos(angle);
    float y2 = m_position.y + (length / 2.0f) * std::sin(angle);

    renderer.drawLine(x1, y1, x2, y2, {139, 0, 0, 255}); // Dark red/maroon line
    Entity::render(renderer); // Call base class render
}

void ProjectileBullet::setOwner(uint32_t ownerId) {
    m_ownerId = ownerId;
}

bool ProjectileBullet::checkMapCollision(const Vec2& nextPos, const EntityContext& context) {
    // This is a simplified check. A real game would use more sophisticated tile-based collision.
    // For now, just check if the next position is outside the map bounds.
    if (!context.mapManager->isMapLoaded()) {
        return false; // No map loaded, no map collision
    }

    // Get map dimensions in pixels
    unsigned mapWidth = context.mapManager->getMapWidthPixels();
    unsigned mapHeight = context.mapManager->getMapHeightPixels();

    // Check if any part of the bullet is outside the map boundaries
    if (nextPos.x < 0 || nextPos.y < 0 ||
        nextPos.x + getSize().x > mapWidth || nextPos.y + getSize().y > mapHeight) {
        return true; // Collision with map boundary
    }

    // TODO: Implement actual collision with map tiles/objects if needed
    // This would involve checking the collision shapes from MapManager
    return false;
}

bool ProjectileBullet::checkEntityCollision(const EntityContext& context) {
    // Iterate through all active entities in the EntityManager
    for (Entity* otherEntity : context.entityManager->getActiveEntities()) {
        // Don't collide with self, owner, or other projectiles (for now)
        if (otherEntity == this || otherEntity->getId() == m_ownerId || otherEntity->getType() == EntityType::PROJECTILE_BULLET) {
            continue;
        }

        // Simple AABB collision check
        if (m_position.x < otherEntity->getPosition().x + otherEntity->getSize().x &&
            m_position.x + getSize().x > otherEntity->getPosition().x &&
            m_position.y < otherEntity->getPosition().y + otherEntity->getSize().y &&
            m_position.y + getSize().y > otherEntity->getPosition().y)
        {
            // Collision detected! Apply damage to the other entity.
            otherEntity->takeDamage(m_damage, m_ownerId);
            return true; // Collision occurred
        }
    }
    return false; // No collision
}

} // namespace TuxArena