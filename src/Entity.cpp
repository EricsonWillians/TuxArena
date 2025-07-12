// src/Entity.cpp
#include "TuxArena/Entity.h"
#include "TuxArena/Log.h"

namespace TuxArena {

Entity::Entity(EntityManager* manager, EntityType type)
    : m_entityManager(manager), m_type(type)
{
}

void Entity::update(const EntityContext& context)
{
    (void)context; // Suppress unused parameter warning
}

void Entity::render(Renderer& renderer)
{
    (void)renderer; // Suppress unused parameter warning
}

// No need for a custom destructor if default is sufficient
// Entity::~Entity() {
//     Log::Info("Entity destroyed: ID " + std::to_string(m_id) + ", Type: " + std::to_string(static_cast<int>(m_type)));
// }

} // namespace TuxArena
