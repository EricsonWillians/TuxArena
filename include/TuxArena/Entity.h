// include/TuxArena/Entity.h
#pragma once

#include <cstdint>   // For uint32_t etc.
#include <string>
#include <vector>
#include <memory>    // For std::unique_ptr in EntityManager, maybe needed here later

// --- Simple Vec2 Math Struct ---
// Replace with a proper math library (GLM, etc.) if more complex math is needed
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    // Basic operations (add more as needed)
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    // float length() const;
    // Vec2 normalized() const;
};


namespace TuxArena {

// Forward declarations of systems Entity might interact with via Context
class Renderer;
class InputManager;
class MapManager;
class EntityManager;
class NetworkClient;
class NetworkServer;
class ModManager;
// class PhysicsWorld; // If using a dedicated physics engine
// class BitStream; // Forward declare bitstream class if used for networking


// Enum to identify different types of entities
// Used for factory creation, collision filtering, etc.
enum class EntityType {
    GENERIC, // Default/placeholder
    PLAYER,
    PROJECTILE_BULLET,
    PROJECTILE_ROCKET, // Example
    ITEM_HEALTH,
    ITEM_AMMO,
    TRIGGER, // Example: invisible trigger volume
    DECORATION, // Example: non-interactive map prop
    // Add more as needed
};


// Context structure passed to entity update methods
// Provides access to relevant game systems and frame data
struct EntityContext {
    float deltaTime = 0.0f;        // Time elapsed since last frame
    bool isServer = false;         // Is this update running on the server?
    InputManager* inputManager = nullptr; // Client only: provides input state
    MapManager* mapManager = nullptr;     // Access to map collision/data
    EntityManager* entityManager = nullptr; // To interact with other entities (spawn, query)
    ModManager* modManager = nullptr;     // Access to modding API/hooks
    NetworkClient* networkClient = nullptr; // Client only: network state/send interface
    NetworkServer* networkServer = nullptr; // Server only: network state/send interface
    // PhysicsWorld* physicsWorld = nullptr; // Physics simulation interface
    // Add other global state or systems as needed
};


// --- Base Entity Class ---
// Abstract base class for all objects managed by the EntityManager.
class Entity {
public:
    // --- Constructor / Destructor ---
    Entity(EntityManager* manager, EntityType type); // Require EntityManager and type
    virtual ~Entity() = default; // Virtual destructor is essential

    // Prevent copying/moving for simplicity unless explicitly needed and handled
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity(Entity&&) = delete;
    Entity& operator=(Entity&&) = delete;


    // --- Core Lifecycle Methods (to be implemented by derived classes) ---

    /**
     * @brief Called once after the entity is created and added to the manager.
     * @param context Provides access to game systems needed for initialization.
     */
    virtual void initialize(const EntityContext& context) {}; // Default empty implementation

    /**
     * @brief Updates the entity's state (movement, logic, AI, etc.).
     * @param context Provides delta time and access to game systems.
     */
    virtual void update(const EntityContext& context) = 0; // Pure virtual

    /**
     * @brief Renders the entity using the provided renderer.
     * @param renderer The rendering interface.
     */
    virtual void render(Renderer& renderer) = 0; // Pure virtual


    // --- Optional Lifecycle / Event Methods ---

    /**
     * @brief Called when this entity collides with another entity (if physics enabled).
     * @param other The other entity involved in the collision.
     * @param result Detailed information about the collision (e.g., impact point, normal).
     */
    // virtual void handleCollision(Entity* other, const CollisionResult& result) {}

    /**
     * @brief Called just before the entity is destroyed (optional cleanup).
     */
    virtual void onDestroy(const EntityContext& context) {};

    /**
     * @brief Serializes the entity's essential state for network transmission.
     * @param stream The output bitstream to write state into.
     * @param isInitialState Is this the full initial state or a delta update?
     */
    // virtual void serializeState(BitStream& stream, bool isInitialState) const {}

    /**
     * @brief Deserializes network state and applies it to the entity.
     * @param stream The input bitstream to read state from.
     * @param timestamp The server timestamp associated with this state.
     */
    // virtual void deserializeState(BitStream& stream, double timestamp) {}


    // --- Accessors ---

    uint32_t getId() const { return m_id; }
    EntityType getType() const { return m_type; }

    const Vec2& getPosition() const { return m_position; }
    void setPosition(const Vec2& pos) { m_position = pos; }
    void setPosition(float x, float y) { m_position.x = x; m_position.y = y; }

    const Vec2& getVelocity() const { return m_velocity; }
    void setVelocity(const Vec2& vel) { m_velocity = vel; }
    void setVelocity(float vx, float vy) { m_velocity.x = vx; m_velocity.y = vy; }

    float getRotation() const { return m_rotation; }
    void setRotation(float angle) { m_rotation = angle; } // Assume degrees or radians consistently

    const Vec2& getSize() const { return m_size; } // Width/Height or Radius/Radius
    void setSize(const Vec2& size) { m_size = size; }
    void setSize(float w, float h) { m_size.x = w; m_size.y = h; }

    bool isActive() const { return m_isActive; }
    void setActive(bool active) { m_isActive = active; }

    bool isStatic() const { return m_isStatic; }
    void setStatic(bool isStatic) { m_isStatic = isStatic; }

    // --- Helper Methods ---

    /**
     * @brief Calculates the Axis-Aligned Bounding Box (AABB) for the entity.
     * Assumes position is center and size is width/height for simplicity. Adjust if needed.
     * @return SDL_FRect representing the AABB.
     */
    // SDL_FRect getAABB() const; // Requires SDL_rect.h - maybe define simple Rect struct?


protected:
    // Allow derived classes access to the manager if absolutely necessary
    // Prefer passing it via context or specific methods.
    EntityManager* m_entityManager = nullptr;

    // --- Core State ---
    uint32_t m_id = 0;            // Unique ID assigned by EntityManager
    EntityType m_type = EntityType::GENERIC;
    Vec2 m_position = {0.0f, 0.0f};
    Vec2 m_velocity = {0.0f, 0.0f};
    float m_rotation = 0.0f;    // Degrees
    Vec2 m_size = {16.0f, 16.0f}; // Default size (e.g., pixels)
    bool m_isActive = true;     // Active and should be updated/rendered
    bool m_isStatic = false;    // Does not move due to physics/velocity

    // Allow EntityManager to set the ID
    friend class EntityManager;
    void setId(uint32_t id) { m_id = id; }
};

} // namespace TuxArena