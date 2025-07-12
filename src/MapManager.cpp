// src/MapManager.cpp
#include "TuxArena/MapManager.h"
#include "TuxArena/Log.h"
#include "TuxArena/Entity.h" // For Vec2, if needed for collision shapes
#include "TuxArena/Constants.h" // For ASSETS_DIR

// Include tmxlite headers needed for implementation
#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/ImageLayer.hpp>
#include <tmxlite/LayerGroup.hpp>

#include "SDL2/SDL_rect.h" // For SDL_Rect return type

#include <filesystem> // For path manipulation (C++17)
#include <algorithm> // For std::transform, std::find_if

namespace TuxArena {

MapManager::MapManager() {
    Log::Info("MapManager created.");
}

MapManager::~MapManager() {
    Log::Info("MapManager destroyed.");
    unloadMap(); // Ensure cleanup
}

bool MapManager::loadMap(const std::string& filePath) {
    unloadMap(); // Unload any previously loaded map

    Log::Info("Attempting to load map from: " + filePath);
    m_mapName = filePath;

    try {
        m_tmxMap.load(filePath);
        m_isMapLoaded = true;
        m_useFallbackMap = false;
    } catch (const std::exception& e) {
        Log::Error("Failed to load TMX map file: " + filePath + ". Error: " + e.what());
        Log::Warning("Attempting to create fallback map.");
        createFallbackMap();
        if (!m_isMapLoaded) {
            Log::Error("Failed to create fallback map. Game cannot proceed without a map.");
            return false;
        }
    }

    if (m_isMapLoaded) {
        // Extract map properties
        m_mapWidth = m_tmxMap.getTileCount().x;
        m_mapHeight = m_tmxMap.getTileCount().y;
        m_tileWidth = m_tmxMap.getTileSize().x;
        m_tileHeight = m_tmxMap.getTileSize().y;

        // Process tilesets
        m_tilesets.clear();
        const auto& tmxTilesets = m_tmxMap.getTilesets();
        for (const auto& tmxTileset : tmxTilesets) {
            TilesetInfo info;
            info.firstGid = tmxTileset.getFirstGID();
            info.tileCount = tmxTileset.getTileCount();
            info.tileWidth = tmxTileset.getTileSize().x;
            info.tileHeight = tmxTileset.getTileSize().y;
            info.columns = tmxTileset.getColumnCount();
            info.spacing = tmxTileset.getSpacing();
            info.margin = tmxTileset.getMargin();
            info.imageWidth = tmxTileset.getImageSize().x;
            info.imageHeight = tmxTileset.getImageSize().y;
            info.imagePath = tmxTileset.getImagePath(); // This is relative to TMX, need to resolve

            // Resolve absolute path for tileset image using ASSETS_DIR
            // Assuming tileset image paths in TMX are relative to the project's assets directory
            // e.g., "tilesets/topdown_tileset.png"
            std::filesystem::path tilesetImagePath = info.imagePath;
            if (tilesetImagePath.is_relative()) {
                info.imagePath = ASSETS_DIR + tilesetImagePath.string();
            } else {
                info.imagePath = tilesetImagePath.string();
            }

            m_tilesets[info.firstGid] = info;
            Log::Info("Loaded tileset: " + info.imagePath + " (First GID: " + std::to_string(info.firstGid) + ")");
        }

        // Process layers (collision objects, spawn points etc.)
        m_collisionShapes.clear();
        m_spawnPoints.clear();
        const auto& layers = m_tmxMap.getLayers();
        for (const auto& layer : layers) {
            processLayer(*layer);
        }

        Log::Info("Map '" + m_mapName + "' loaded successfully. Dimensions: " + std::to_string(m_mapWidth) + "x" + std::to_string(m_mapHeight) + " tiles, " + std::to_string(m_tileWidth) + "x" + std::to_string(m_tileHeight) + " tile size.");
    }

    return m_isMapLoaded;
}

void MapManager::createFallbackMap() {
    Log::Info("Creating fallback map...");

    m_mapName = "fallback_map";
    m_mapWidth = 32; // Example: 32x32 tiles
    m_mapHeight = 24;
    m_tileWidth = 32; // Example: 32x32 pixels per tile
    m_tileHeight = 32;
    m_isMapLoaded = true;
    m_useFallbackMap = true;

    // Clear existing data
    m_tmxMap = tmx::Map(); // Reset tmx::Map object
    m_tilesets.clear();
    m_collisionShapes.clear();
    m_spawnPoints.clear();

    // Add a simple collision boundary
    // Top wall
    m_collisionShapes.push_back(CollisionShape(CollisionShape::Type::Rectangle, 0, 0, (float)getMapWidthPixels(), (float)m_tileHeight));
    // Bottom wall
    m_collisionShapes.push_back(CollisionShape(CollisionShape::Type::Rectangle, 0, (float)getMapHeightPixels() - m_tileHeight, (float)getMapWidthPixels(), (float)getMapHeightPixels()));
    // Left wall
    m_collisionShapes.push_back(CollisionShape(CollisionShape::Type::Rectangle, 0, 0, (float)m_tileWidth, (float)getMapHeightPixels()));
    // Right wall
    m_collisionShapes.push_back(CollisionShape(CollisionShape::Type::Rectangle, (float)getMapWidthPixels() - m_tileWidth, 0, (float)getMapWidthPixels(), (float)getMapHeightPixels()));

    // Add a default spawn point in the center
    m_spawnPoints.push_back({(float)getMapWidthPixels() / 2.0f, (float)getMapHeightPixels() / 2.0f, "player_spawn", "player"});

    Log::Info("Fallback map created. Dimensions: " + std::to_string(m_mapWidth) + "x" + std::to_string(m_mapHeight) + " tiles, " + std::to_string(m_tileWidth) + "x" + std::to_string(m_tileHeight) + " tile size.");
}

void MapManager::unloadMap() {
    if (m_isMapLoaded) {
        Log::Info("Unloading map: " + m_mapName);
        m_tmxMap = tmx::Map(); // Clears the map data
        m_isMapLoaded = false;
        m_mapName = "";
        m_mapWidth = 0;
        m_mapHeight = 0;
        m_tileWidth = 0;
        m_tileHeight = 0;
        m_tilesets.clear();
        m_collisionShapes.clear();
        m_spawnPoints.clear();
        m_useFallbackMap = false;
    }
}


void MapManager::processLayer(const tmx::Layer& layer) {
     // Recursively process group layers
     if (layer.getType() == tmx::Layer::Type::Group) {
          Log::Info("  - Processing Group Layer: " + layer.getName());
          const auto& groupLayer = layer.getLayerAs<tmx::LayerGroup>();
          for (const auto& subLayer : groupLayer.getLayers()) {
              processLayer(*subLayer);
          }
     }
     // Process object layers for collisions and spawns
     else if (layer.getType() == tmx::Layer::Type::Object) {
          Log::Info("  - Processing Object Layer: " + layer.getName());
          const auto& objectLayer = layer.getLayerAs<tmx::ObjectGroup>();
          processObjectLayer(objectLayer); // Extract both collision and spawns here
     }
     // Handle other layer types (Tile, Image) if necessary
     else if (layer.getType() == tmx::Layer::Type::Tile) {
          Log::Info("  - Found Tile Layer: " + layer.getName() + " (Data used by Renderer)");
     }
}


void MapManager::processObjectLayer(const tmx::ObjectGroup& group) {
    Log::Info("    - Extracting objects from layer: " + group.getName());
    for (const auto& object : group.getObjects()) {
        // Check object type property for "SpawnPoint" or similar
        std::string objectType = object.getType();
        std::string lowerObjectType = objectType;
        std::transform(lowerObjectType.begin(), lowerObjectType.end(), lowerObjectType.begin(), ::tolower);

        bool isSpawn = (lowerObjectType.find("spawn") != std::string::npos);
        // Assume anything else MIGHT be collision, unless explicitly ignored type
        bool isCollision = !isSpawn && (object.getShape() != tmx::Object::Shape::Point && object.getShape() != tmx::Object::Shape::Text);

        if (isSpawn) {
             SpawnPoint spawn;
             spawn.x = object.getPosition().x;
             spawn.y = object.getPosition().y;
             spawn.name = object.getName();
             spawn.type = objectType; // Store original type casing
             m_spawnPoints.push_back(spawn);
             Log::Info("      - Found Spawn Point: " + spawn.name + " (" + spawn.type + ") at (" + std::to_string(spawn.x) + "," + std::to_string(spawn.y) + ")");

        } else if (isCollision) {
             CollisionShape shape(CollisionShape::Type::Rectangle, object.getPosition().x, object.getPosition().y, object.getPosition().x + object.getAABB().width, object.getPosition().y + object.getAABB().height);
             shape.minX = object.getAABB().left;
             shape.minY = object.getAABB().top;
             shape.maxX = object.getAABB().left + object.getAABB().width;
             shape.maxY = object.getAABB().top + object.getAABB().height;

             const auto& pos = object.getPosition();
             const auto objShape = object.getShape();

             if (objShape == tmx::Object::Shape::Rectangle) {
                 shape.type = CollisionShape::Type::Rectangle;
                 float w = object.getAABB().width;
                 float h = object.getAABB().height;
                 shape.points.push_back({pos.x, pos.y});
                 shape.points.push_back({pos.x + w, pos.y});
                 shape.points.push_back({pos.x + w, pos.y + h});
                 shape.points.push_back({pos.x, pos.y + h});
             } else if (objShape == tmx::Object::Shape::Ellipse) {
                  shape.type = CollisionShape::Type::Ellipse;
                  float w = object.getAABB().width;
                  float h = object.getAABB().height;
                  shape.points.push_back({pos.x, pos.y}); // Store AABB points for consistency
                  shape.points.push_back({pos.x + w, pos.y});
                  shape.points.push_back({pos.x + w, pos.y + h});
                  shape.points.push_back({pos.x, pos.y + h});
                  Log::Info("      - Found Ellipse collision (using AABB): " + object.getName());
             } else if (objShape == tmx::Object::Shape::Polygon) {
                 shape.type = CollisionShape::Type::Polygon;
                 const auto& polyPoints = object.getPoints();
                 for(const auto& p : polyPoints) {
                     shape.points.push_back({pos.x + p.x, pos.y + p.y}); // Points relative to object pos
                 }
             } else if (objShape == tmx::Object::Shape::Polyline) {
                  shape.type = CollisionShape::Type::Polyline;
                  const auto& linePoints = object.getPoints();
                  for(const auto& p : linePoints) {
                      shape.points.push_back({pos.x + p.x, pos.y + p.y});
                  }
             }

             if (!shape.points.empty()) {
                  m_collisionShapes.push_back(shape);
                  Log::Info("      - Added collision shape: " + object.getName() + " (Type: " + std::to_string(static_cast<int>(shape.type)) + ")");
             }
        } // end if isCollision

    } // end object loop
}


// --- Map Property Accessors ---
unsigned MapManager::getMapWidthTiles() const {
    if (m_useFallbackMap) return m_mapWidth;
    return m_tmxMap.getTileCount().x;
}

unsigned MapManager::getMapHeightTiles() const {
    if (m_useFallbackMap) return m_mapHeight;
    return m_tmxMap.getTileCount().y;
}

unsigned MapManager::getTileWidth() const {
    if (m_useFallbackMap) return m_tileWidth;
    return m_tmxMap.getTileSize().x;
}

unsigned MapManager::getTileHeight() const {
    if (m_useFallbackMap) return m_tileHeight;
    return m_tmxMap.getTileSize().y;
}

unsigned MapManager::getMapWidthPixels() const {
    if (m_useFallbackMap) return m_mapWidth * m_tileWidth;
    return m_tmxMap.getTileCount().x * m_tmxMap.getTileSize().x;
}

unsigned MapManager::getMapHeightPixels() const {
    if (m_useFallbackMap) return m_mapHeight * m_tileHeight;
    return m_tmxMap.getTileCount().y * m_tmxMap.getTileSize().y;
}

const std::vector<tmx::Layer::Ptr>& MapManager::getRawLayers() const {
    if (m_useFallbackMap) {
        static const std::vector<tmx::Layer::Ptr> emptyLayers; // Return empty for fallback
        return emptyLayers;
    }
    return m_tmxMap.getLayers();
}

const TilesetInfo* MapManager::findTilesetForGid(unsigned gid) const {
    if (m_useFallbackMap) return nullptr; // No tilesets for fallback map
    // ... existing logic ...
    for (const auto& pair : m_tilesets) {
        const TilesetInfo& ts = pair.second;
        if (gid >= ts.firstGid && gid < (ts.firstGid + ts.tileCount)) {
            return &ts;
        }
    }
    return nullptr;
}

SDL_Rect MapManager::getSourceRectForGid(unsigned gid, const TilesetInfo& tilesetInfo) const {
    if (m_useFallbackMap) return {0,0,0,0}; // Return empty rect for fallback
    // ... existing logic ...
    unsigned tileId = gid - tilesetInfo.firstGid;
    unsigned x = (tileId % tilesetInfo.columns) * (tilesetInfo.tileWidth + tilesetInfo.spacing);
    unsigned y = (tileId / tilesetInfo.columns) * (tilesetInfo.tileHeight + tilesetInfo.spacing);
    return {(int)x, (int)y, (int)tilesetInfo.tileWidth, (int)tilesetInfo.tileHeight};
}


} // namespace TuxArena
