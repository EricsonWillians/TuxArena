// src/Player.cpp
#include "TuxArena/Player.h"
#include "TuxArena/EntityManager.h" // For spawning projectiles
#include "TuxArena/InputManager.h"  // For reading input
#include "TuxArena/Renderer.h"    // For rendering the player
#include "TuxArena/MapManager.h"    // For collision detection
#include "TuxArena/Entity.h"      // For EntityType::PROJECTILE_BULLET (define later)

#include "SDL3/SDL.h" // For SDL_Log, potentially timing/math functions

#include <cmath> // For std::atan2, std::cos, std::sin

namespace TuxArena {

// Define EntityType for projectiles if not done elsewhere yet
// Ideally, these would be consolidated.
// enum class EntityType { PROJECTILE_BULLET }; // Already in Entity.h

Player::Player(EntityManager* manager) :
    Entity(manager, EntityType::PLAYER) // Call base constructor, specify type
{
    // Initialize player-specific defaults
    setSize(32.0f, 32.0f); // Example size in pixels
    m_health = 100;
    m_shootTimer = 0.0f; // Ready to shoot initially
    // std::cout << "Player entity created." << std::endl;
}

void Player::initialize(const EntityContext& context) {
    // Called by EntityManager after creation and basic setup
    // std::cout << "Player ID " << getId() << " initialized." << std::endl;
    // Pre-load assets here? Or handle in first render/update?
    // For now, texture loading is deferred to the render method.
}

void Player::update(const EntityContext& context) {
    // --- Timers ---
    if (m_shootTimer > 0.0f) {
        m_shootTimer -= context.deltaTime;
    }

    // --- Input ---
    // Read input and set internal movement/action intentions
    handleInput(context);

    // --- Actions ---
    attemptShoot(context);

    // --- Movement & Collision ---
    // Calculate movement based on intentions and handle map collisions
    applyMovement(context);

    // --- Aiming ---
    // Update aim direction (e.g., towards mouse cursor if applicable)
    if (context.inputManager && !context.isServer) { // Client-side aiming towards mouse
        float mouseX, mouseY;
        context.inputManager->getMousePosition(mouseX, mouseY);
        // Convert mouse screen coordinates to world coordinates if camera exists
        // Assuming direct correlation for now:
        float dx = mouseX - m_position.x;
        float dy = mouseY - m_position.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.01f) { // Avoid division by zero / normalize only if mouse moved significantly
             m_aimDirection.x = dx / len;
             m_aimDirection.y = dy / len;
             // Optionally, set player rotation to match aim direction
             // m_rotation = std::atan2(m_aimDirection.y, m_aimDirection.x) * (180.0f / M_PI); // Convert radians to degrees
        }
    } else {
        // Server or non-mouse aim: Aim direction might follow rotation instead
        // float angleRad = m_rotation * (M_PI / 180.0f);
        // m_aimDirection = { std::cos(angleRad), std::sin(angleRad) };
    }

    // --- Other Logic ---
    // Health regeneration, animation updates, etc.
}

void Player::handleInput(const EntityContext& context) {
    if (!context.inputManager || context.isServer) {
        // No input handling on the server side directly for player movement
        // Server would receive client input via network messages
        m_moveInput = {0.0f, 0.0f};
        m_rotationInput = 0.0f;
        return;
    }

    // --- Movement Input ---
    m_moveInput = {0.0f, 0.0f}; // Reset intention each frame
    if (context.inputManager->isActionPressed(GameAction::MOVE_FORWARD)) {
        m_moveInput.y -= 1.0f; // Assuming -Y is forward in top-down
    }
    if (context.inputManager->isActionPressed(GameAction::MOVE_BACKWARD)) {
        m_moveInput.y += 1.0f;
    }
    if (context.inputManager->isActionPressed(GameAction::MOVE_LEFT)) {
        m_moveInput.x -= 1.0f;
    }
    if (context.inputManager->isActionPressed(GameAction::MOVE_RIGHT)) {
        m_moveInput.x += 1.0f;
    }

    // Normalize diagonal movement vector to prevent faster diagonal speed
    float lenSq = m_moveInput.x * m_moveInput.x + m_moveInput.y * m_moveInput.y;
    if (lenSq > 1.0f) {
        float len = std::sqrt(lenSq);
        m_moveInput.x /= len;
        m_moveInput.y /= len;
    }

    // --- Rotation Input (Keyboard Example - Overridden by Mouse Aim if Active) ---
    // Uncomment if keyboard rotation is desired instead of mouse aim setting rotation
    /*
    m_rotationInput = 0.0f;
    if (context.inputManager->isActionPressed(GameAction::ROTATE_LEFT)) { // Assume ROTATE actions exist
        m_rotationInput -= 1.0f;
    }
    if (context.inputManager->isActionPressed(GameAction::ROTATE_RIGHT)) {
        m_rotationInput += 1.0f;
    }
    */

    // Sprinting could modify m_moveSpeed here
    // if (context.inputManager->isActionPressed(GameAction::SPRINT)) { ... }
}

void Player::applyMovement(const EntityContext& context) {
    // --- Rotation ---
    // Apply keyboard rotation if used
    // m_rotation += m_rotationInput * m_rotationSpeed * context.deltaTime;
    // Keep rotation within 0-360 degrees (optional)
    // m_rotation = std::fmod(m_rotation, 360.0f);
    // if (m_rotation < 0.0f) m_rotation += 360.0f;

    // Rotation determined by mouse aim direction (calculated in update)
    // Convert aim direction back to angle if needed for rendering/logic
     m_rotation = std::atan2(m_aimDirection.y, m_aimDirection.x) * (180.0f / M_PI);


    // --- Position ---
    Vec2 velocity = {m_moveInput.x * m_moveSpeed, m_moveInput.y * m_moveSpeed};
    Vec2 deltaPos = velocity * context.deltaTime;
    Vec2 currentPos = getPosition();
    Vec2 nextPos = currentPos + deltaPos;

    // --- Collision ---
    // Check against map collision shapes BEFORE updating position
    if (context.mapManager && (std::abs(deltaPos.x) > 0.001f || std::abs(deltaPos.y) > 0.001f)) {
         nextPos = resolveMapCollision(currentPos, nextPos, context);
    }

    // --- Update State ---
    setPosition(nextPos);
    // Set velocity state if needed for physics/networking
    // setVelocity(velocity); // This might be simplified velocity based only on input
}


Vec2 Player::resolveMapCollision(const Vec2& currentPos, const Vec2& nextPos, const EntityContext& context) {
     if (!context.mapManager) return nextPos; // No map, no collision

     // Simple AABB vs Polygon/Rect collision detection
     // Player AABB calculation (assuming position is center)
     float halfWidth = m_size.x / 2.0f;
     float halfHeight = m_size.y / 2.0f;
     SDL_FRect playerRectNext = {
         nextPos.x - halfWidth,
         nextPos.y - halfHeight,
         m_size.x,
         m_size.y
     };

     Vec2 resolvedPos = nextPos; // Start with intended position

     const auto& collisionShapes = context.mapManager->getCollisionShapes();
     for (const auto& shape : collisionShapes) {
         // Broadphase check (optional but recommended for performance)
         SDL_FRect shapeAABB = { shape.minX, shape.minY, shape.maxX - shape.minX, shape.maxY - shape.minY };
         if (!SDL_HasRectIntersectionFloat(&playerRectNext, &shapeAABB)) {
             continue; // No intersection with shape's bounding box
         }

         // Narrowphase check (AABB vs AABB for now, needs AABB vs Polygon later)
         // For simplicity, just check AABB intersection again
          if (SDL_HasRectIntersectionFloat(&playerRectNext, &shapeAABB)) {
              // Collision detected! Simple resolution: Stop movement by reverting to currentPos.
              // A better approach involves calculating penetration depth and sliding.
              // For now, just prevent movement into the object. This is jerky.
              // Try resolving axis by axis:
              Vec2 moveAttemptX = {nextPos.x, currentPos.y};
              Vec2 moveAttemptY = {currentPos.x, nextPos.y};

              SDL_FRect playerRectX = playerRectNext; playerRectX.y = currentPos.y - halfHeight;
              SDL_FRect playerRectY = playerRectNext; playerRectY.x = currentPos.x - halfWidth;


              bool collisionX = false;
               if (SDL_HasRectIntersectionFloat(&playerRectX, &shapeAABB)) {
                   collisionX = true;
               }

              bool collisionY = false;
               if (SDL_HasRectIntersectionFloat(&playerRectY, &shapeAABB)) {
                   collisionY = true;
               }

              // Block movement on the axis that collided
              if(collisionX) {
                  resolvedPos.x = currentPos.x; // Block X movement
                  playerRectNext.x = currentPos.x - halfWidth; // Update rect for Y check
              }
              if(collisionY) {
                  resolvedPos.y = currentPos.y; // Block Y movement
                  playerRectNext.y = currentPos.y - halfHeight; // Update rect just in case
              }

              // If still colliding after resolving one axis (e.g., corner collision), block both?
              // This basic axis separation is still prone to issues. A proper SAT or penetration vector
              // calculation is needed for smooth sliding.
              // For now, if we blocked X and Y independently, we might still be inside.
              // Let's just stick with the axis-blocking version:
              // resolvedPos is now the position after trying to move X then Y and blocking if necessary.

              // Update playerRectNext based on resolvedPos for next iteration
              playerRectNext.x = resolvedPos.x - halfWidth;
              playerRectNext.y = resolvedPos.y - halfHeight;

              // If both axes were blocked, we are essentially back at currentPos for this shape interaction.
          }
     }

     return resolvedPos; // Return the potentially adjusted position
}


void Player::attemptShoot(const EntityContext& context) {
    if (!context.inputManager || !context.entityManager || context.isServer) {
        return; // Shooting initiated by client input, requires EntityManager
    }

    if (context.inputManager->isActionPressed(GameAction::SHOOT) && m_shootTimer <= 0.0f) {
        // Reset cooldown timer
        m_shootTimer = m_shootCooldown;

        // Calculate spawn position (e.g., slightly in front of the player center)
        float spawnDist = m_size.x / 2.0f + 5.0f; // Spawn 5 pixels ahead of radius
        Vec2 spawnPos = m_position + (m_aimDirection * spawnDist);

        // Calculate bullet velocity
        float bulletSpeed = 500.0f;
        Vec2 bulletVel = m_aimDirection * bulletSpeed;

        // Create the bullet entity (requires ProjectileBullet type definition)
        EntityContext bulletContext = context; // Pass context along?
        context.entityManager->createEntity(
            EntityType::PROJECTILE_BULLET, // Need to define this type
            spawnPos,
            bulletContext, // Pass context for initialize if needed
            m_rotation,  // Initial rotation matches player/aim
            bulletVel    // Initial velocity
            // Use default bullet size or specify here
        );

        // std::cout << "Player ID " << getId() << " shot." << std::endl;
        // Play sound effect (requires audio system)
        // context.audioManager->playSound("shoot.wav");
    }
}

void Player::render(Renderer& renderer) {
    // Load texture if not already loaded
    if (!m_playerTexture) {
        m_playerTexture = renderer.loadTexture(m_texturePath);
        if (!m_playerTexture) {
            std::cerr << "ERROR: Failed to load player texture: " << m_texturePath << std::endl;
            // Optionally draw a placeholder shape if texture fails
            SDL_FRect errorRect = {m_position.x - m_size.x / 2.0f, m_position.y - m_size.y / 2.0f, m_size.x, m_size.y};
            renderer.drawRect(&errorRect, {255, 0, 0, 255}); // Red rectangle
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

    // Draw the player texture rotated
    renderer.drawTexture(m_playerTexture, nullptr, &dstRect, m_rotation, SDL_FLIP_NONE);

    // --- Optional: Render Debug Info ---
    #if defined(DEBUG_RENDER) || 1 // Enable debug rendering easily for now
        // Draw aim direction line
        Vec2 lineEnd = m_position + (m_aimDirection * m_size.x); // Line length based on size
        // Need a drawLine method in Renderer
        // renderer.drawLine(m_position.x, m_position.y, lineEnd.x, lineEnd.y, {0, 255, 0, 255}); // Green line

        // Draw bounding box
        // renderer.drawRectOutline(&dstRect, {255, 255, 0, 150}); // Yellow outline
    #endif
}

void Player::onDestroy(const EntityContext& context) {
    // std::cout << "Player ID " << getId() << " destroyed." << std::endl;
    // Release resources if necessary (e.g., textures loaded specifically by this instance - unlikely here)
    // Texture is cached by Renderer, so no need to free m_playerTexture here.
}


} // namespace TuxArena