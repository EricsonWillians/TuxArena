#include <TuxArena/EntityManager.h> // Header for this implementation file

// Standard Library Includes
#include <algorithm> // For std::remove_if, std::find_if
#include <cmath>     // For std::sqrt in distance calculations
#include <iostream>  // Used by the basic Log helper
#include <memory>    // For std::unique_ptr, std::make_unique
#include <set>       // For efficient processing of destruction queue
#include <stdexcept> // For std::runtime_error (optional for more severe errors)

// Project-Specific Includes
#include "TuxArena/Entity.h"      // Base entity class and EntityContext
#include "TuxArena/MapManager.h"  // Needed for initialization
#include "TuxArena/Renderer.h"    // Needed for render methods

// --- Include Headers for ALL Derived Entity Types ---
// These are required for the factory method (createEntity).
#include "TuxArena/Player.h"
#include "TuxArena/ProjectileBullet.h"
// #include "TuxArena/ItemHealth.h"     // Add other entity types as they are created
// #include "TuxArena/TriggerVolume.h"


namespace TuxArena {

// --- Basic Logging Helper (Replace with a real logging library) ---
namespace Log {
    static void Info(const std::string& msg) { std::cout << "[INFO] " << msg << std::endl; }
    static void Warning(const std::string& msg) { std::cerr << "[WARN] " << msg << std::endl; }
    static void Error(const std::string& msg) { std::cerr << "[ERROR] " << msg << std::endl; }
} // namespace Log
// --- End Basic Logging Helper ---


// --- Constructor / Destructor ---

EntityManager::EntityManager() {
    // Log::Info("EntityManager created.");
}

EntityManager::~EntityManager() {
    // Log::Info("EntityManager destroyed.");
    // Ensure shutdown is called if the user forgets, although explicit call is preferred.
    if (m_isInitialized) {
        Log::Warning("EntityManager destroyed without explicit shutdown() call. Cleaning up...");
        shutdown();
    }
}

// --- Initialization / Shutdown ---

bool EntityManager::initialize(MapManager* mapManager) {
    if (m_isInitialized) {
        Log::Warning("EntityManager::initialize called multiple times.");
        return true;
    }
    // Log::Info("Initializing EntityManager...");
    m_mapManager = mapManager; // Store pointer to map manager

    // Reset state in case initialize is called after a shutdown
    m_entities.clear();
    m_entityMap.clear();
    m_destructionQueue.clear();
    m_nextEntityId = 1; // Reset ID counter

    m_isInitialized = true;
    // Log::Info("EntityManager initialized successfully.");
    return true;
}

void EntityManager::shutdown() {
    if (!m_isInitialized) {
        return;
    }
    // Log::Info("Shutting down EntityManager...");

    // Process any pending destructions first, calling onDestroy hooks
    // Note: Requires a valid context. We might use the last stored one or a default.
    // If called outside the main loop, context might be invalid.
    // For simplicity, we'll clear directly here. A more robust shutdown
    // might process the queue one last time if a valid context can be provided.
    // processDestructionQueue(); // Requires valid context

    // Clear entity collections. unique_ptr handles deletion.
    m_entityMap.clear();   // Clear the lookup map first
    m_entities.clear();    // This destroys all owned Entity objects
    m_destructionQueue.clear(); // Clear any remaining queued IDs

    m_mapManager = nullptr; // Release pointer to map manager
    m_isInitialized = false;
    // Log::Info("EntityManager shutdown complete.");
}

// --- Entity Creation / Destruction ---

Entity* EntityManager::createEntity(EntityType type,
                                    const Vec2& position,
                                    const EntityContext& context, // Context needed for initialize()
                                    float rotation,
                                    const Vec2& velocity,
                                    const Vec2& size)
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
                newEntity = std::make_unique<ProjectileBullet>(this);
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
    uint32_t newId = assignNextId();
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


    // Log::Info("Created Entity ID: " + std::to_string(newId) + ", Type: " + std::to_string(static_cast<int>(type)));
    return rawPtr; // Return the non-owning raw pointer
}

void EntityManager::destroyEntity(uint32_t id) {
    if (!m_isInitialized || id == 0) return; // Ignore requests for invalid ID 0

    // Check if the entity exists before queueing
    if (m_entityMap.count(id)) {
         // Avoid adding duplicates to the queue if destroyEntity is called multiple times per frame
         // Using find is more efficient on vector than iterating manually if queue grows large
         if (std::find(m_destructionQueue.begin(), m_destructionQueue.end(), id) == m_destructionQueue.end()) {
              // Log::Info("Queueing Entity ID " + std::to_string(id) + " for destruction.");
              m_destructionQueue.push_back(id);
              // Optional: Immediately mark as inactive. This prevents further updates/renders this frame.
              // Useful if destruction should be immediate visually/logically, even if memory cleanup is deferred.
              // if (Entity* entity = getEntityById(id)) { // Use getEntityById for safety
              //     entity->setActive(false);
              // }
         }
    } else {
         // Log::Warning("Attempted to destroy non-existent or already destroyed entity ID: " + std::to_string(id));
    }
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
    if (!m_isInitialized) return;

    // Store the context in case onDestroy needs it during processDestructionQueue
    m_lastUpdateContext = context;

    // Update all active entities.
    // Using an index loop avoids iterator invalidation if entities are added during update,
    // although ideally entities shouldn't add others directly in their update loop
    // without careful consideration. It processes only entities present at the start.
    size_t initialSize = m_entities.size();
    for (size_t i = 0; i < initialSize; ++i) {
        // Use .get() on unique_ptr, check for null just in case (shouldn't happen)
        if (Entity* entity = m_entities[i].get()) {
            // Only update active entities that are NOT already queued for destruction
            if (entity->isActive()) {
                 bool destructionQueued = false;
                 // Check destruction queue efficiently (consider set lookup if queue gets large)
                 for (uint32_t idToDestroy : m_destructionQueue) {
                      if (entity->getId() == idToDestroy) {
                           destructionQueued = true;
                           break;
                      }
                 }

                 if (!destructionQueued) {
                     try {
                         entity->update(context);
                     } catch (const std::exception& e) {
                         Log::Error("Exception during entity update() for ID " + std::to_string(entity->getId()) + ": " + e.what());
                         // Consider destroying the problematic entity to prevent further errors
                         // destroyEntity(entity->getId());
                     }
                 }
            }
        }
    }

    // Process entities marked for destruction AFTER the main update loop
    processDestructionQueue();
}

void EntityManager::render(Renderer& renderer) {
    if (!m_isInitialized) return;

    // Render all active entities. Add view culling for optimization later.
    for (const auto& entityPtr : m_entities) {
        // Check unique_ptr itself and the entity's active status
        if (entityPtr && entityPtr->isActive()) {
            // TODO: Add frustum/view culling check here
            // if (renderer.isVisible(entityPtr->getAABB())) {
            try {
                entityPtr->render(renderer);
            } catch (const std::exception& e) {
                Log::Error("Exception during entity render() for ID " + std::to_string(entityPtr->getId()) + ": " + e.what());
                // Don't destroy based on render error, might be temporary graphics issue
            }
            // }
        }
    }
}

void EntityManager::renderDebug(Renderer& renderer) {
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
      // Log::Info("Rendered debug info for " + std::to_string(m_entities.size()) + " potential entities.");
}

// --- Internal Methods ---

uint32_t EntityManager::assignNextId() {
    // WARNING: Server Authoritative IDs Required for Networking!
    // This simple incrementing scheme is ONLY suitable for single-player
    // or the server-side instance in a multiplayer game. Clients must
    // use IDs assigned by the server.

    // Basic overflow check (extremely unlikely with uint32_t, but good practice)
    if (m_nextEntityId == 0) { // Wrapped around or started at 0
        Log::Warning("Entity ID counter wrapped around or started at 0. Resetting to 1.");
        m_nextEntityId = 1;
        // Potentially check if ID 1 is already in use if wrapping is possible,
        // but this indicates a *very* long-running game or excessive entity creation.
        if (m_entityMap.count(m_nextEntityId)) {
             Log::Error("CRITICAL: Entity ID counter wrapped and collided with existing ID 1!");
             // This is a fatal error scenario in most cases.
             // Throw exception or handle appropriately.
             // For now, return 0 to signal failure.
             return 0;
        }
    }
    // TODO: Add check for maximum entity count if needed.
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
                // Log::Info("Destroying Entity ID: " + std::to_string(entityPtr->getId()));
                return true; // Mark for removal by std::remove_if
            }
            return false; // Keep this entity
        });

    // Erase the elements from the returned iterator to the end.
    // unique_ptr's destructor handles deleting the Entity object.
    if (removalIt != m_entities.end()) {
        // Log::Info("Erasing " + std::to_string(std::distance(removalIt, m_entities.end())) + " entities from vector.");
        m_entities.erase(removalIt, m_entities.end());
    }

    // Clear the queue now that processing is done
    m_destructionQueue.clear();
}

// --- Query Method Implementations ---

std::vector<Entity*> EntityManager::getActiveEntities() const {
    if (!m_isInitialized) return {};
    std::vector<Entity*> activeList;
    activeList.reserve(m_entities.size()); // Avoid reallocations
    for (const auto& entityPtr : m_entities) {
        // Ensure the pointer itself is valid and the entity is active
        if (entityPtr && entityPtr->isActive()) {
            activeList.push_back(entityPtr.get());
        }
    }
    return activeList;
}

std::vector<Entity*> EntityManager::findEntitiesInRadius(
    const Vec2& center,
    float radius,
    const std::function<bool(const Entity*)>& filter) const
{
    if (!m_isInitialized || radius < 0.0f) return {};

    std::vector<Entity*> foundList;
    // Optimization: Use squared radius to avoid sqrt calculation in the loop
    float radiusSq = radius * radius;

    // O(N) iteration. For large numbers of entities, consider spatial partitioning
    // (e.g., Quadtree, Spatial Hash Grid) to reduce the number of entities checked.
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


} // namespace TuxArena