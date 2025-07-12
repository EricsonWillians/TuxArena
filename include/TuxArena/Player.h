// include/TuxArena/Player.h
#pragma once

#include "TuxArena/Entity.h"
#include "TuxArena/Weapon.h"
#include <string>
#include <vector>
#include <memory>
#include <SDL_render.h> // For SDL_Texture

#include <SDL_render.h> // For SDL_Texture

#include <SDL_render.h> // For SDL_Texture

#include <SDL_render.h> // For SDL_Texture

// Forward Declarations
namespace TuxArena {
    class Renderer;
}


namespace TuxArena {

class Weapon;

class Player : public Entity {
public:
    // Constructor takes the EntityManager pointer required by the base Entity class
    explicit Player(EntityManager* manager); // Use explicit to prevent implicit conversions

    // --- Overridden Virtual Methods ---
    void initialize(const EntityContext& context) override;
    void update(const EntityContext& context) override;
    void render(Renderer& renderer) override;
    void onDestroy(const EntityContext& context) override;
    void takeDamage(float damage, uint32_t instigatorId) override;
    // virtual void handleCollision(Entity* other, const CollisionResult& result) override; // If needed
    // virtual void serializeState(BitStream& stream, bool isInitialState) const override; // If needed
    // virtual void deserializeState(BitStream& stream, double timestamp) override;       // If needed


private:
    // --- Player-Specific Logic/State ---

    // Movement & Rotation
    float m_moveSpeed = 200.0f;      // Pixels per second
    float m_rotationSpeed = 270.0f;  // Degrees per second (adjust as needed)
    Vec2 m_moveInput = {0.0f, 0.0f}; // Stores {-1, 0, 1} for x and y input axes
    float m_rotationInput = 0.0f;    // Stores {-1, 0, 1} for rotation input

    // Aiming (example using mouse)
    Vec2 m_aimDirection = {1.0f, 0.0f}; // Direction player is aiming (normalized)

    // Combat
    std::vector<std::unique_ptr<Weapon>> m_weapons;
    int m_currentWeaponIndex = -1;
    int m_health = 100;
    bool m_shootInput = false;
    // Add ammo, score, etc.

    // Weapon management
    void addWeapon(std::unique_ptr<Weapon> weapon);
    void switchWeapon(int slotIndex);
    Weapon* getCurrentWeapon() const;

    // Rendering
    SDL_Texture* m_playerTexture = nullptr; // Cached texture pointer
    std::string m_texturePath = "assets/characters/tux.png"; // Default texture path

    // --- Private Helper Methods ---

    /**
     * @brief Reads input from InputManager and updates internal intention states.
     * @param context Provides access to InputManager.
     */
    void handleInput(const EntityContext& context);

    /**
     * @brief Applies movement and rotation based on input and delta time, handling collisions.
     * @param context Provides delta time and MapManager access.
     */
    void applyMovement(const EntityContext& context);

    /**
     * @brief Handles the shooting action if input is detected and cooldown permits.
     * @param context Provides access to EntityManager to spawn projectiles.
     */
    void attemptShoot(const EntityContext& context);

     /**
      * @brief Performs collision detection against the map geometry.
      * @param nextPos The intended position after movement.
      * @param context Provides MapManager access.
      * @return The adjusted position after collision resolution (or original if no collision).
      */
     Vec2 resolveMapCollision(const Vec2& currentPos, const Vec2& nextPos, const EntityContext& context);
};

} // namespace TuxArena