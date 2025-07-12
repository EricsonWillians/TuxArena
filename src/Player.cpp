// src/Player.cpp
#include "TuxArena/Player.h"
#include "TuxArena/InputManager.h"
#include "TuxArena/Renderer.h"
#include "TuxArena/EntityManager.h"
#include "TuxArena/ProjectileBullet.h"
#include "TuxArena/Log.h"
#include "TuxArena/MapManager.h"
#include "TuxArena/ParticleManager.h"

#include <cmath> // For std::sin, std::cos, std::atan2, std::sqrt
#include <algorithm> // For std::min, std::max

namespace TuxArena {

Player::Player(EntityManager* manager)
    : Entity(manager, EntityType::PLAYER) {
    Log::Info("Player instance created.");
    setSize(32.0f, 32.0f); // Default player size
}

void Player::initialize(const EntityContext& context) {
    Log::Info("Player initialized.");

    // Load player texture and stats based on selected character
    if (context.modManager && !context.playerCharacterId.empty()) {
        const CharacterInfo* selectedChar = context.modManager->getCharacterDefinition(context.playerCharacterId);
        if (selectedChar) {
            m_playerTexture = context.renderer->loadTexture(selectedChar->texturePath);
            if (!m_playerTexture) {
                Log::Error("Failed to load texture for selected character: " + selectedChar->texturePath);
            }
            m_health = selectedChar->health;
            m_moveSpeed = selectedChar->speed;
            Log::Info("Player stats loaded from character: " + selectedChar->name + ", Health: " + std::to_string(m_health) + ", Speed: " + std::to_string(m_moveSpeed));
        } else {
            Log::Warning("Selected character ID '" + context.playerCharacterId + "' not found in ModManager. Using default stats and texture.");
            // Fallback to default texture and stats
            m_playerTexture = context.renderer->loadTexture(m_texturePath);
            if (!m_playerTexture) {
                Log::Error("Failed to load default player texture: " + m_texturePath);
            }
            m_health = 100.0f;
            m_moveSpeed = 200.0f;
        }
    } else if (context.renderer) {
        // Fallback to default if no character selected or modManager is null
        m_playerTexture = context.renderer->loadTexture(m_texturePath);
        if (!m_playerTexture) {
            Log::Error("Failed to load default player texture: " + m_texturePath);
        }
        m_health = 100.0f;
        m_moveSpeed = 200.0f;
    }

    // Add weapons from ModManager
    if (context.modManager) {
        const auto& weaponDefs = context.modManager->getWeaponDefinitions();
        if (weaponDefs.empty()) {
            Log::Warning("No weapon definitions loaded by ModManager. Adding hardcoded defaults.");
            // Add hardcoded defaults if no mods provide weapons
            WeaponDef pistol_def;
            pistol_def.name = "Pistol";
            pistol_def.fireRate = 5.0f;
            pistol_def.projectileDamage = 10.0f;
            addWeapon(std::make_unique<Weapon>(pistol_def, this));

            WeaponDef shotgun_def;
            shotgun_def.type = WeaponType::SHOTGUN;
            shotgun_def.name = "Shotgun";
            shotgun_def.fireRate = 1.5f;
            shotgun_def.projectilesPerShot = 8;
            shotgun_def.spreadAngle = 20.0f;
            shotgun_def.projectileDamage = 8.0f;
            addWeapon(std::make_unique<Weapon>(shotgun_def, this));
        } else {
            for (const auto& pair : weaponDefs) {
                addWeapon(std::make_unique<Weapon>(pair.second, this));
            }
        }
    }

    if (!m_weapons.empty()) {
        m_currentWeaponIndex = 0;
    } else {
        m_currentWeaponIndex = -1;
    }
}

void Player::update(const EntityContext& context) {
    // Only process input if this is the client-controlled player
    if (!context.isServer && context.inputManager) {
        handleInput(context);
    }

    applyMovement(context);

    // Update current weapon
    if (m_currentWeaponIndex != -1) {
        m_weapons[m_currentWeaponIndex]->update(context);
    }

    // Attempt to shoot if input is active
    if (m_shootInput) {
        attemptShoot(context);
    }
}

void Player::render(Renderer& renderer) {
    if (m_playerTexture) {
        SDL_FRect dstRect = {
            m_position.x - m_size.x / 2.0f,
            m_position.y - m_size.y / 2.0f,
            m_size.x,
            m_size.y
        };
        // Render with rotation
        renderer.drawTexture(m_playerTexture, nullptr, &dstRect, m_rotation);
    } else {
        // Fallback: draw a colored rectangle if texture not loaded
        SDL_FRect rect = {
            m_position.x - m_size.x / 2.0f,
            m_position.y - m_size.y / 2.0f,
            m_size.x,
            m_size.y
        };
        renderer.drawRect(&rect, {0, 255, 0, 255}, true); // Green rectangle
    }
}

void Player::onDestroy(const EntityContext& context) {
    (void)context; // Suppress unused parameter warning
}

void Player::handleInput(const EntityContext& context) {
    m_moveInput = {0.0f, 0.0f};
    m_shootInput = false;

    if (context.inputManager->isActionPressed(GameAction::MOVE_FORWARD)) {
        m_moveInput.y -= 1.0f;
    }
    if (context.inputManager->isActionPressed(GameAction::MOVE_BACKWARD)) {
        m_moveInput.y += 1.0f;
    }
    if (context.inputManager->isActionPressed(GameAction::STRAFE_LEFT)) {
        m_moveInput.x -= 1.0f;
    }
    if (context.inputManager->isActionPressed(GameAction::STRAFE_RIGHT)) {
        m_moveInput.x += 1.0f;
    }

    // Rotation input
    if (context.inputManager->isActionPressed(GameAction::TURN_LEFT)) {
        m_rotationInput -= 1.0f;
    }
    if (context.inputManager->isActionPressed(GameAction::TURN_RIGHT)) {
        m_rotationInput += 1.0f;
    }

    // Normalize movement input if diagonal movement is faster
    float moveLength = std::sqrt(m_moveInput.x * m_moveInput.x + m_moveInput.y * m_moveInput.y);
    if (moveLength > 1.0f) {
        m_moveInput.x /= moveLength;
        m_moveInput.y /= moveLength;
    }

    // Mouse aiming
    float mouseX, mouseY;
    context.inputManager->getMousePosition(mouseX, mouseY);
    // Calculate direction from player to mouse
    float dx = mouseX - m_position.x;
    float dy = mouseY - m_position.y;
    m_rotation = std::atan2(dy, dx) * (180.0f / M_PI); // Convert radians to degrees

    // Shooting input
    if (context.inputManager->isActionPressed(GameAction::FIRE_PRIMARY)) {
        m_shootInput = true;
    }

    // Weapon switching input
    if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_1)) {
        switchWeapon(0);
    } else if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_2)) {
        switchWeapon(1);
    } else if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_3)) {
        switchWeapon(2);
    } else if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_4)) {
        switchWeapon(3);
    } else if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_5)) {
        switchWeapon(4);
    } else if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_6)) {
        switchWeapon(5);
    } else if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_7)) {
        switchWeapon(6);
    } else if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_8)) {
        switchWeapon(7);
    } else if (context.inputManager->isActionPressed(GameAction::WEAPON_SLOT_9)) {
        switchWeapon(8);
    }
}

void Player::applyMovement(const EntityContext& context) {
    // Apply rotation
    if (m_rotationInput != 0.0f) {
        m_rotation += m_rotationSpeed * context.deltaTime * m_rotationInput;
        // Keep rotation within 0-360 degrees
        if (m_rotation >= 360.0f) m_rotation -= 360.0f;
        if (m_rotation < 0.0f) m_rotation += 360.0f;
    }

    Vec2 currentPos = m_position;
    Vec2 desiredVelocity = {0.0f, 0.0f};

    // Convert rotation to radians for trigonometric functions
    float rotationRad = m_rotation * (M_PI / 180.0f);

    // Calculate forward/backward movement
    if (m_moveInput.y != 0.0f) {
        desiredVelocity.x += m_moveInput.y * std::cos(rotationRad) * m_moveSpeed;
        desiredVelocity.y += m_moveInput.y * std::sin(rotationRad) * m_moveSpeed;
    }

    // Calculate strafing movement (90 degrees to the right of current rotation)
    if (m_moveInput.x != 0.0f) {
        float strafeRotationRad = (m_rotation + 90.0f) * (M_PI / 180.0f);
        desiredVelocity.x += m_moveInput.x * std::cos(strafeRotationRad) * m_moveSpeed;
        desiredVelocity.y += m_moveInput.x * std::sin(strafeRotationRad) * m_moveSpeed;
    }

    Vec2 nextPos = {currentPos.x + desiredVelocity.x * context.deltaTime, currentPos.y + desiredVelocity.y * context.deltaTime};

    // Basic map boundary collision (AABB vs AABB for map bounds)
    if (context.mapManager) {
        float mapWidth = static_cast<float>(context.mapManager->getMapWidthPixels());
        float mapHeight = static_cast<float>(context.mapManager->getMapHeightPixels());

        // Clamp player position within map boundaries
        nextPos.x = std::max(m_size.x / 2.0f, std::min(nextPos.x, mapWidth - m_size.x / 2.0f));
        nextPos.y = std::max(m_size.y / 2.0f, std::min(nextPos.y, mapHeight - m_size.y / 2.0f));
    }

    m_position = nextPos;
}

void Player::attemptShoot(const EntityContext& context) {
    if (m_currentWeaponIndex != -1) {
        m_weapons[m_currentWeaponIndex]->shoot(context);
    }
}

void Player::takeDamage(float damage, uint32_t instigatorId) {
    (void)instigatorId; // Suppress unused parameter warning
    m_health -= damage;
    Log::Info("Player took " + std::to_string(damage) + " damage, health is now " + std::to_string(m_health));
    if (m_health <= 0) {
        Log::Info("Player has died.");
        // Handle death
    }

    // Emit blood particles
    if (m_entityManager) {
        m_entityManager->getParticleManager()->emitBlood(m_position.x, m_position.y, 20);
    }
}

void Player::addWeapon(std::unique_ptr<Weapon> weapon) {
    m_weapons.push_back(std::move(weapon));
    if (m_currentWeaponIndex == -1) {
        m_currentWeaponIndex = 0; // Equip the first weapon added
    }
}

void Player::switchWeapon(int slotIndex) {
    if (slotIndex >= 0 && static_cast<size_t>(slotIndex) < m_weapons.size()) {
        m_currentWeaponIndex = slotIndex;
        Log::Info("Switched to weapon: " + m_weapons[m_currentWeaponIndex]->getDefinition().name);
    } else {
        Log::Warning("Attempted to switch to invalid weapon slot: " + std::to_string(slotIndex));
    }
}

Weapon* Player::getCurrentWeapon() const {
    if (m_currentWeaponIndex != -1 && static_cast<size_t>(m_currentWeaponIndex) < m_weapons.size()) {
        return m_weapons[m_currentWeaponIndex].get();
    }
    return nullptr;
}

} // namespace TuxArena