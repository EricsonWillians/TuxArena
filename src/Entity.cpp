// src/Entity.cpp
#include "TuxArena/Entity.h"
#include "SDL3/SDL_rect.h" // If getAABB() is implemented here

// Other includes if needed for base class logic (rare)

namespace TuxArena {

// --- Constructor ---
// Requires the EntityManager that created it and its type.
Entity::Entity(EntityManager* manager, EntityType type) :
    m_entityManager(manager), // Store pointer to the manager
    m_type(type),
    m_isActive(true), // Entities are active by default when created
    m_isStatic(false) // Assume dynamic by default
{
    // ID is assigned by the EntityManager after creation
}

// --- Virtual Destructor ---
// Default implementation is sufficient unless the base class owns raw resources.
// Entity::~Entity() = default; // Already defaulted in header


// --- Optional Base Implementations ---
// If you add non-pure virtual methods like initialize, handleCollision, onDestroy,
// serializeState, deserializeState, you can provide default (often empty)
// implementations here.

// Example: Default initialize does nothing.
// void Entity::initialize(const EntityContext& context) {
//     // Base implementation (does nothing)
// }

// Example: Default onDestroy does nothing.
// void Entity::onDestroy(const EntityContext& context) {
//    // Base implementation (does nothing)
// }


// --- Helper Methods ---

// Example implementation of getAABB if needed in the base class
/*
SDL_FRect Entity::getAABB() const {
    // Assumes m_position is the center and m_size is width/height
    float halfWidth = m_size.x / 2.0f;
    float halfHeight = m_size.y / 2.0f;
    return SDL_FRect{
        m_position.x - halfWidth,
        m_position.y - halfHeight,
        m_size.x,
        m_size.y
    };
    // Adjust calculation if m_position represents top-left or if rotation matters for AABB
}
*/

} // namespace TuxArena