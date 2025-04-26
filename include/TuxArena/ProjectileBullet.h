// include/TuxArena/ProjectileBullet.h
#pragma once

#include "TuxArena/Entity.h"
#include <string>
#include <cstdint>

// Forward Declarations
struct SDL_Texture;
namespace TuxArena {
    class Renderer;
}

namespace TuxArena {

class ProjectileBullet : public Entity {
public:
    explicit ProjectileBullet(EntityManager* manager);

    // --- Overridden Virtual Methods ---
    void initialize(const EntityContext& context) override;
    void update(const EntityContext& context) override;
    void render(Renderer& renderer) override;
    // void onDestroy(const EntityContext& context) override; // Optional: For effects on destruction
    // virtual void handleCollision(Entity* other, const CollisionResult& result) override; // Potentially use this

    /**
     * @brief Sets the owner of this projectile.
     * @param ownerId The unique ID of the entity that fired this projectile.
     */
    void setOwner(uint32_t ownerId);


private:
    // --- Bullet-Specific State ---
    float m_lifetime = 2.0f;   // Seconds before self-destructing
    float m_lifeTimer = 0.0f;  // Current time alive
    int m_damage = 10;         // Damage dealt on hit
    uint32_t m_ownerId = 0;    // ID of the entity that fired this (prevents self-collision)

    // Rendering
    SDL_Texture* m_texture = nullptr; // Cached texture pointer
    std::string m_texturePath = "assets/projectiles/bullet.png"; // Example path

    // --- Helper Methods ---
     /**
      * @brief Checks for and handles collisions with map geometry.
      * @param nextPos The intended position after movement.
      * @param context Provides MapManager access.
      * @return True if a collision occurred, false otherwise.
      */
     bool checkMapCollision(const Vec2& nextPos, const EntityContext& context);

      /**
       * @brief Checks for and handles collisions with other entities.
       * @param context Provides EntityManager access.
       * @return True if a collision occurred that destroyed the bullet, false otherwise.
       */
     bool checkEntityCollision(const EntityContext& context);

};

} // namespace TuxArena