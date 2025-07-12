// src/EntityManager.cpp
#include "TuxArena/EntityManager.h" // Header for this implementation file

// Standard Library Includes
#include <algorithm> // For std::remove_if, std::find_if
#include <cmath>     // For std::sqrt in distance calculations
#include <iostream>  // Used by the basic Log helper
#include <memory>    // For std::unique_ptr, std::make_unique
#include <set>       // For efficient processing of destruction queue
#include <stdexcept> // For std::runtime_error (optional for more severe errors)
#include <functional> // For std::function

// Project-Specific Includes
#include "TuxArena/Entity.h"      // Base entity class and EntityContext
#include "TuxArena/MapManager.h"  // Needed for initialization
#include "TuxArena/Renderer.h"    // Needed for render methods
#include "TuxArena/Log.h"

// --- Include Headers for ALL Derived Entity Types ---
// These are required for the factory method (createEntity).
#include "TuxArena/Player.h"
#include "TuxArena/ProjectileBullet.h"
// #include "TuxArena/ItemHealth.h"     // Add other entity types as they are created
// #include "TuxArena/TriggerVolume.h"


namespace TuxArena {

// --- Constructor / Destructor ---

EntityManager::EntityManager() {
    Log::Info("EntityManager created.");
}

EntityManager::~EntityManager() {
    Log::Info("EntityManager destroyed.");
    // Ensure shutdown is called if the user forgets, although explicit call is preferred.
    if (m_isInitialized) {
        Log::Warning("EntityManager destroyed without explicit shutdown() call. Cleaning up...");
        shutdown();
    }
}

bool EntityManager::initialize(MapManager* mapManager) {
    Log::Info("EntityManager::initialize() called.");
    if (m_isInitialized) {
        Log::Warning("EntityManager::initialize called multiple times.");
        return true;
    }
    Log::Info("Initializing EntityManager...");
    m_mapManager = mapManager; // Store pointer to map manager

    // Reset state in case initialize is called after a shutdown
    m_entities.clear();
    m_entityMap.clear();
    m_destructionQueue.clear();
    m_nextEntityId = 1; // Reset ID counter

    m_isInitialized = true;
    Log::Info("EntityManager initialized successfully.");
    return true;
}

void EntityManager::shutdown() {
    if (!m_isInitialized) {
        return;
    }
    Log::Info("Shutting down EntityManager...");

    clearAllEntities(); // Use the new clear function for consistency

    m_mapManager = nullptr; // Release pointer to map manager
    m_isInitialized = false;
    Log::Info("EntityManager shutdown complete.");
}

void EntityManager::clearAllEntities() {
    Log::Info("Clearing all entities from EntityManager...");
    // Process any pending destructions first, calling onDestroy hooks
    // This is important to ensure entities clean up their resources before being deleted.
    processDestructionQueue();

    // Clear entity collections. unique_ptr handles deletion.
    m_entityMap.clear();   // Clear the lookup map first
    m_entities.clear();    // This destroys all owned Entity objects
    m_destructionQueue.clear(); // Clear any remaining queued IDs
    m_nextEntityId = 1; // Reset ID counter after clearing all entities
    m_player = nullptr; // Ensure player pointer is null after clearing
}

// --- Entity Creation / Destruction ---

Entity* EntityManager::createEntity(EntityType type,
                                    const Vec2& position,
                                    const EntityContext& context, // Context needed for initialize()
                                    float rotation,
                                    const Vec2& velocity,
                                    const Vec2& size,
                                    uint32_t forceId) // New parameter to force ID
{
    if (!m_isInitialized) {
        Log::Error("EntityManager::createEntity called before initialization.");
        return nullptr;
    }

    std::unique_ptr<Entity> newEntity = nullptr;

    // --- Factory Logic ---
    // Consider replacing with a registration pattern or function map for more extensibility
    // if the number of entity types becomes very large. Switch is fine for moderate numbers.
    try {
        switch (type) {
            case EntityType::PLAYER:
                newEntity = std::make_unique<Player>(this);
                break;
            case EntityType::PROJECTILE_BULLET:
                newEntity = std::make_unique<ProjectileBullet>(this, &m_particleManager, position.x, position.y, rotation, velocity.x, size.x); // Assuming velocity.x is speed, size.x is damage
                break;
            // Add cases for other entity types here...
            // case EntityType::ITEM_HEALTH:
            //     newEntity = std::make_unique<ItemHealth>(this);
            //     break;
            case EntityType::GENERIC: // Fall-through intentional
            default:
                Log::Error("Attempted to create entity of unknown or generic type: " + std::to_string(static_cast<int>(type)));
                return nullptr; // Explicitly return nullptr for unhandled types
        }
    } catch (const std::bad_alloc& e) {
        Log::Error("Memory allocation failed for new entity of type " + std::to_string(static_cast<int>(type)) + ": " + e.what());
        return nullptr;
    } catch (const std::exception& e) {
        Log::Error("Exception during entity construction for type " + std::to_string(static_cast<int>(type)) + ": " + e.what());
        return nullptr;
    }

    // --- Assign ID and Initial State ---
    uint32_t newId = (forceId == 0) ? assignNextId() : forceId; // Use forced ID or assign new
    if (newId == 0) { // Check if ID assignment failed (e.g., wrapped around and hit 0)
        Log::Error("Failed to assign a valid new entity ID.");
        return nullptr; // Cannot proceed without a valid ID
    }

    // Use protected setId method via friend declaration
    newEntity->setId(newId);
    newEntity->setPosition(position);
    newEntity->setRotation(rotation);
    newEntity->setVelocity(velocity);
    newEntity->setSize(size);
    newEntity->setActive(true); // Ensure active by default

    // --- Call Entity's Initialize ---
    try {
        newEntity->initialize(context);
    } catch (const std::exception& e) {
        Log::Error("Exception during entity initialize() for ID " + std::to_string(newId) + ": " + e.what());
        // If initialize fails, we should probably not add the entity
        return nullptr;
    }

    // --- Add to Collections ---
    Entity* rawPtr = newEntity.get(); // Get raw pointer before moving unique_ptr
    try {
        m_entities.push_back(std::move(newEntity)); // Move ownership to the vector
        m_entityMap[newId] = rawPtr; // Store raw pointer in the lookup map
    } catch (const std::exception& e) {
         Log::Error("Exception occurred while adding entity ID " + std::to_string(newId) + " to collections: " + e.what());
         // Attempt to clean up partially added state if possible (tricky)
         // If push_back failed, newEntity unique_ptr still owns memory and will delete it.
         // If map insertion failed after push_back, need to remove from vector.
         // Best to ensure vector/map don't throw exceptions under normal conditions (e.g., sufficient memory).
         return nullptr;
    }


    Log::Info("Created Entity ID: " + std::to_string(newId) + ", Type: " + std::to_string(static_cast<int>(type)));
    return rawPtr; // Return the non-owning raw pointer
}

void EntityManager::destroyEntity(uint32_t id) {
    m_destructionQueue.push_back(id);
}

void EntityManager::addEntity(std::unique_ptr<Entity> entity) {
    uint32_t id = assignNextId();
    entity->setId(id);
    entity->initialize(m_lastUpdateContext); // Initialize with the last known context
    m_entityMap[id] = entity.get();
    m_entities.push_back(std::move(entity));
    Log::Info("Added entity with ID: " + std::to_string(id) + " to EntityManager.");
}

Entity* EntityManager::getEntityById(uint32_t id) const {
     if (!m_isInitialized || id == 0) return nullptr;

    auto it = m_entityMap.find(id);
    if (it != m_entityMap.end()) {
        // Check if the pointer is valid (paranoid check, should always be valid if in map)
        // and potentially if it's active, depending on desired behavior.
        // Returning inactive entities might be useful for some queries.
        return it->second;
    }
    return nullptr;
}

// --- Update / Render ---

void EntityManager::update(const EntityContext& context) {
    m_lastUpdateContext = context; // Store for deferred destruction

    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->update(context);
        }
    }

    m_particleManager.update(context.deltaTime);

    processDestructionQueue();
}

void EntityManager::render(Renderer& renderer) {
    // Render all active entities
    for (const auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->render(renderer);
        }
    }

    m_particleManager.render(renderer);
}

void EntityManager::renderDebug(Renderer& renderer) {
     (void)renderer; // Suppress unused parameter warning
     if (!m_isInitialized) return;
     // Render debug info for all active entities
     for (const auto& entityPtr : m_entities) {
         if (entityPtr && entityPtr->isActive()) {
             // Assuming entities might have a renderDebug method
             // entityPtr->renderDebug(renderer);

             // Example: Draw bounding boxes using a helper if available
             // SDL_FRect aabb = entityPtr->getAABB(); // Assuming getAABB exists
             // renderer.drawRectOutline(&aabb, {0, 255, 0, 150}); // Green outline
         }
     }
      Log::Info("Rendered debug info for " + std::to_string(m_entities.size()) + " potential entities.");
}

std::vector<Entity*> EntityManager::getActiveEntities() const {
    std::vector<Entity*> active_entities;
    for (const auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            active_entities.push_back(entity.get());
        }
    }
    return active_entities;
}

// --- Internal Methods ---

uint32_t EntityManager::assignNextId() {
    if (m_nextEntityId == 0) { // Check if ID counter wrapped around and hit 0
        Log::Warning("Entity ID counter wrapped around or started at 0. Resetting to 1.");
        m_nextEntityId = 1;
    }
    return m_nextEntityId++;
}

void EntityManager::processDestructionQueue() {
    if (m_destructionQueue.empty() || !m_isInitialized) {
        return;
    }

    // Use a set for efficient O(log N) lookup of IDs to destroy, faster than O(N) vector search
    // if the queue or entity list is large. For small numbers, vector search is fine.
    std::set<uint32_t> idsToDestroy(m_destructionQueue.begin(), m_destructionQueue.end());

    // Use std::remove_if to partition the vector. It moves elements *not* matching
    // the predicate (i.e., elements to keep) to the beginning.
    // The predicate should return TRUE for elements to be REMOVED.
    auto removalIt = std::remove_if(m_entities.begin(), m_entities.end(),
        [&](const std::unique_ptr<Entity>& entityPtr) -> bool {
            if (!entityPtr) return true; // Remove nullptrs if they somehow exist

            bool shouldRemove = idsToDestroy.count(entityPtr->getId());
            if (shouldRemove) {
                // Call onDestroy hook BEFORE removing from map and vector
                try {
                    // Use the context stored from the last update call
                    entityPtr->onDestroy(m_lastUpdateContext);
                } catch (const std::exception& e) {
                    Log::Error("Exception during entity onDestroy() for ID " + std::to_string(entityPtr->getId()) + ": " + e.what());
                }
                // Remove from the lookup map immediately
                m_entityMap.erase(entityPtr->getId());
                Log::Info("Destroying Entity ID: " + std::to_string(entityPtr->getId()));
                return true; // Mark for removal by std::remove_if
            }
            return false; // Keep this entity
        });

    // Erase the elements from the returned iterator to the end.
    // unique_ptr's destructor handles deleting the Entity object.
    if (removalIt != m_entities.end()) {
        Log::Info("Erasing " + std::to_string(std::distance(removalIt, m_entities.end())) + " entities from vector.");
        m_entities.erase(removalIt, m_entities.end());
    }

    // Clear the queue now that processing is done
    m_destructionQueue.clear();
}

// --- Query Method Implementations ---



std::vector<Entity*> EntityManager::findEntitiesInRadius(
    const Vec2& center,
    float radius,
    const std::function<bool(const Entity*)>& filter) const
{
    if (!m_isInitialized || radius < 0.0f) return {};

    // Optimization: Use squared radius to avoid sqrt calculation in the loop
    float radiusSq = radius * radius;

    // O(N) iteration. For large numbers of entities, consider spatial partitioning
    // (e.g., Quadtree, Spatial Hash Grid) to reduce the number of entities checked.
    std::vector<Entity*> foundList;
    for (const auto& entityPtr : m_entities) {
        if (entityPtr && entityPtr->isActive()) {   
            // Calculate squared distance from entity center to query center
            const Vec2& pos = entityPtr->getPosition();
            float dx = pos.x - center.x;
            float dy = pos.y - center.y;
            float distSq = (dx * dx) + (dy * dy);

            // Check if within squared radius
            if (distSq <= radiusSq) {
                 // Apply optional filter predicate
                 if (!filter || filter(entityPtr.get())) {
                     foundList.push_back(entityPtr.get());
                 }
            }
        }
    }
    return foundList;
}

Player* EntityManager::getPlayer() {
    // Simple implementation: find the first player entity
    for (const auto& entityPtr : m_entities) {
        if (entityPtr && entityPtr->getType() == EntityType::PLAYER) {
            return static_cast<Player*>(entityPtr.get());
        }
    }
    return nullptr;
}

} // namespace TuxArena