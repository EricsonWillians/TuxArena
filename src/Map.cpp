// src/Map.cpp
#include "TuxArena/Map.h"
#include "SDL3/SDL_rect.h" // For SDL_Rect return type

// Include tmxlite headers needed for implementation
#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/ImageLayer.hpp>
#include <tmxlite/GroupLayer.hpp>

#include <iostream> // For logging
#include <filesystem> // For path manipulation (C++17)
#include <algorithm> // For std::transform, std::find_if

namespace TuxArena {

// Helper to combine paths correctly (consider std::filesystem::path methods)
std::string joinPaths(const std::string& p1, const std::string& p2) {
    if (p1.empty() || p2.empty()) {
        return p1.empty() ? p2 : p1;
    }
    // Use std::filesystem for more robust path joining
    try {
        std::filesystem::path path1(p1);
        std::filesystem::path path2(p2);
        // If p2 is absolute, it replaces p1. If relative, it appends.
        return (path1 / path2).lexically_normal().string();
    } catch (const std::exception& e) {
        std::cerr << "Filesystem path error joining '" << p1 << "' and '" << p2 << "': " << e.what() << std::endl;
        // Fallback to basic string concat as a last resort
        char lastCharP1 = p1.back();
        if (lastCharP1 == '/' || lastCharP1 == '\\') {
            return p1 + p2;
        } else {
            return p1 + '/' + p2; // Assume '/' separator
        }
    }
}


MapManager::MapManager() {
    // std::cout << "MapManager created." << std::endl; // Logging can be added
}

MapManager::~MapManager() {
    // std::cout << "MapManager destroyed." << std::endl;
    unloadMap(); // Ensure cleanup
}

bool MapManager::loadMap(const std::string& filePath, const std::string& baseAssetPath) {
    if (m_mapLoaded) {
        // std::cout << "Unloading previous map before loading new one." << std::endl;
        unloadMap();
    }

    // std::cout << "Loading map: " << filePath << std::endl;
    m_baseAssetPath = baseAssetPath; // Store base asset path if provided

    // Attempt to load the TMX file using tmxlite
    if (!m_tmxMap.load(filePath)) {
        std::cerr << "ERROR: Failed to load TMX map file: " << filePath << std::endl;
        return false;
    }

     // Determine the directory containing the map file to resolve relative paths
     try {
         std::filesystem::path mapPath(filePath);
         // Check if path is empty or invalid before getting parent
         if (mapPath.has_parent_path()) {
            m_mapDirectory = mapPath.parent_path().string();
         } else {
            m_mapDirectory = "."; // Default if it's just a filename
         }
     } catch (const std::exception& e) {
         std::cerr << "Warning: Could not determine map directory from path '" << filePath << "': " << e.what() << std::endl;
         m_mapDirectory = "."; // Default to current directory
     }
     // std::cout << "Map directory resolved to: " << m_mapDirectory << std::endl;


    // --- Process Tilesets ---
    m_tilesets.clear();
    const auto& tilesets = m_tmxMap.getTilesets();
    // std::cout << "Found " << tilesets.size() << " tilesets." << std::endl;
    for (const auto& ts : tilesets) {
        TilesetInfo info;
        info.firstGid = ts.getFirstGID();
        info.tileCount = ts.getTileCount();
        info.tileWidth = ts.getTileSize().x;
        info.tileHeight = ts.getTileSize().y;
        info.columns = ts.getColumnCount();
        info.spacing = ts.getSpacing();
        info.margin = ts.getMargin();
        info.imageWidth = ts.getImageSize().x;
        info.imageHeight = ts.getImageSize().y;

        // Resolve image path: relative paths in TMX are relative to the TMX file itself.
        std::string imagePathInTmx = ts.getImagePath();
        if (!imagePathInTmx.empty()) {
             try {
                 std::filesystem::path relativeImagePath(imagePathInTmx);
                 std::filesystem::path mapDir(m_mapDirectory);
                 // Join map directory with the relative path from TMX
                 std::filesystem::path absoluteImagePath = (mapDir / relativeImagePath).lexically_normal();

                 // Now, potentially make it relative to the baseAssetPath if needed,
                 // or just store the resolved path relative to the project execution.
                 // Assuming the resolved path is what the Renderer needs:
                 info.imagePath = absoluteImagePath.string();

                // If using baseAssetPath: check if absoluteImagePath needs modification
                // This logic depends heavily on how assets are structured/deployed.
                // Example: if baseAssetPath is "/opt/TuxArena/assets" and resolved path is
                // "/home/user/workspace/TuxArena/maps/resources/tileset.png", you might
                // need a more complex mapping based on project structure.
                // For now, we store the path resolved relative to the TMX file.
             } catch (const std::exception& e) {
                  std::cerr << "Error resolving tileset image path '" << imagePathInTmx << "': " << e.what() << std::endl;
                  info.imagePath = imagePathInTmx; // Fallback to original path
             }
        } else {
             std::cerr << "Warning: Tileset '" << ts.getName() << "' has no image path." << std::endl;
        }


        m_tilesets[info.firstGid] = info;
        // Debug log for processed tileset info can be added here
    }

    // --- Process Layers (Collision, Spawns, etc.) ---
    m_collisionShapes.clear();
    m_spawnPoints.clear();
    const auto& layers = m_tmxMap.getLayers();
    // std::cout << "Processing " << layers.size() << " layers/groups..." << std::endl;
    for (const auto& layer : layers) {
         processLayer(*layer); // Process recursively
    }

    // std::cout << "Map processing complete. Found " << m_collisionShapes.size() << " collision shapes and " << m_spawnPoints.size() << " spawn points." << std::endl;
    m_mapLoaded = true;
    return true;
}

void MapManager::unloadMap() {
    if (!m_mapLoaded) return;
    // std::cout << "Unloading map data..." << std::endl;
    m_collisionShapes.clear();
    m_spawnPoints.clear();
    m_tilesets.clear();
    m_tmxMap = tmx::Map(); // Reset tmxlite map object
    m_mapDirectory = "";
    m_baseAssetPath = "";
    m_mapLoaded = false;
    // std::cout << "Map data unloaded." << std::endl;
}


void MapManager::processLayer(const tmx::Layer& layer) {
     // Recursively process group layers
     if (layer.getType() == tmx::Layer::Type::Group) {
          // std::cout << "  - Processing Group Layer: " << layer.getName() << std::endl;
          const auto& groupLayer = layer.getLayerAs<tmx::GroupLayer>();
          for (const auto& subLayer : groupLayer.getLayers()) {
              processLayer(*subLayer);
          }
     }
     // Process object layers for collisions and spawns
     else if (layer.getType() == tmx::Layer::Type::Object) {
          // std::cout << "  - Processing Object Layer: " << layer.getName() << std::endl;
          const auto& objectLayer = layer.getLayerAs<tmx::ObjectGroup>();
          processObjectLayer(objectLayer); // Extract both collision and spawns here

          // Optionally, use separate layer processing if needed based on layer names/props
          // std::string lowerLayerName = layer.getName();
          // std::transform(lowerLayerName.begin(), lowerLayerName.end(), lowerLayerName.begin(), ::tolower);
          // if (lowerLayerName.find("collision") != std::string::npos) { processObjectLayer(objectLayer); }
          // if (lowerLayerName.find("spawn") != std::string::npos) { processSpawnpointLayer(objectLayer); }
     }
     // Handle other layer types (Tile, Image) if necessary
     // else if (layer.getType() == tmx::Layer::Type::Tile) {
     //      std::cout << "  - Found Tile Layer: " << layer.getName() << " (Data used by Renderer)" << std::endl;
     // }
}


void MapManager::processObjectLayer(const tmx::ObjectGroup& group) {
    // std::cout << "    - Extracting objects from layer: " << group.getName() << std::endl;
    for (const auto& object : group.getObjects()) {
        // Check object type property for "SpawnPoint" or similar
        std::string objectType = object.getType();
        std::string lowerObjectType = objectType;
        std::transform(lowerObjectType.begin(), lowerObjectType.end(), lowerObjectType.begin(), ::tolower);

        bool isSpawn = (lowerObjectType.find("spawn") != std::string::npos);
        // Assume anything else MIGHT be collision, unless explicitly ignored type
        bool isCollision = !isSpawn && (object.getShape() != tmx::Object::Shape::Point && object.getShape() != tmx::Object::Shape::Text);
        // Refine collision check: maybe look for a "Collision=true" property?

        if (isSpawn) {
             SpawnPoint spawn;
             spawn.x = object.getPosition().x;
             spawn.y = object.getPosition().y;
             // Adjust spawn point origin if needed (e.g., from top-left to center)
             // spawn.x += object.getAABB().width / 2.0f;
             // spawn.y += object.getAABB().height / 2.0f;
             spawn.name = object.getName();
             spawn.type = objectType; // Store original type casing
             m_spawnPoints.push_back(spawn);
             // std::cout << "      - Found Spawn Point: " << spawn.name << " (" << spawn.type << ") at (" << spawn.x << "," << spawn.y << ")" << std::endl;

        } else if (isCollision) {
             CollisionShape shape;
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
                  // Treat ellipse as its AABB for simple physics, or store ellipse params
                  shape.type = CollisionShape::Type::Ellipse;
                  float w = object.getAABB().width;
                  float h = object.getAABB().height;
                  shape.points.push_back({pos.x, pos.y}); // Store AABB points for consistency
                  shape.points.push_back({pos.x + w, pos.y});
                  shape.points.push_back({pos.x + w, pos.y + h});
                  shape.points.push_back({pos.x, pos.y + h});
                  // Store ellipse center/radii if needed: pos.x + w/2, pos.y + h/2, w/2, h/2
                  // std::cout << "      - Found Ellipse collision (using AABB): " << object.getName() << std::endl;
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
                   // std::cout << "      - Added collision shape: " << object.getName() << " (Type: " << (int)shape.type << ")" << std::endl;
             }
        } // end if isCollision

    } // end object loop
}

// Optional: separate function if spawn points guaranteed to be in specific layers
// void MapManager::processSpawnpointLayer(const tmx::ObjectGroup& group) { ... }


// --- Map Property Accessors ---
unsigned MapManager::getMapWidthTiles() const {
    return m_mapLoaded ? m_tmxMap.getTileCount().x : 0;
}

unsigned MapManager::getMapHeightTiles() const {
    return m_mapLoaded ? m_tmxMap.getTileCount().y : 0;
}

unsigned MapManager::getTileWidth() const {
    return m_mapLoaded ? m_tmxMap.getTileSize().x : 0;
}

unsigned MapManager::getTileHeight() const {
     return m_mapLoaded ? m_tmxMap.getTileSize().y : 0;
}

unsigned MapManager::getMapWidthPixels() const {
     return m_mapLoaded ? getMapWidthTiles() * getTileWidth() : 0;
}

unsigned MapManager::getMapHeightPixels() const {
     return m_mapLoaded ? getMapHeightTiles() * getTileHeight() : 0;
}

// --- Data Accessors ---

const std::vector<tmx::Layer::Ptr>& MapManager::getRawLayers() const {
    return m_tmxMap.getLayers();
}

const TilesetInfo* MapManager::findTilesetForGid(unsigned gid) const {
    if (gid == 0 || !m_mapLoaded) return nullptr;
    // tmxlite provides this mapping directly via the map object
    const auto& tileset = m_tmxMap.getTilesetForGID(gid);
    if (tileset) {
        // Find our corresponding cached TilesetInfo using the first GID
        auto it = m_tilesets.find(tileset->getFirstGID());
        if (it != m_tilesets.end()) {
            return &(it->second);
        } else {
             std::cerr << "ERROR: tmxlite found tileset for GID " << gid << " but it's not in our processed cache!" << std::endl;
        }
    }
    return nullptr;

    /* Manual search fallback (less efficient):
    const TilesetInfo* foundTileset = nullptr;
    unsigned maxFirstGid = 0;
    for (const auto& [firstGid, tilesetInfo] : m_tilesets) {
        if (gid >= firstGid && gid < (firstGid + tilesetInfo.tileCount)) { // Check if GID is within this tileset's range
             if (firstGid >= maxFirstGid) { // Prefer the tileset starting with the highest GID <= gid
                 maxFirstGid = firstGid;
                 foundTileset = &tilesetInfo;
             }
        }
    }
    return foundTileset;
    */
}

SDL_Rect MapManager::getSourceRectForGid(unsigned gid, const TilesetInfo& tilesetInfo) const {
    if (gid == 0 || !m_mapLoaded || tilesetInfo.tileWidth <= 0 || tilesetInfo.tileHeight <= 0) {
        return {0, 0, 0, 0};
    }

    // Calculate the local ID within the tileset
    unsigned localId = gid - tilesetInfo.firstGid;

    // Calculate the row and column in the tileset grid
    // Use tilesetInfo.columns provided by tmxlite if > 0, otherwise calculate
    unsigned numCols = tilesetInfo.columns;
    if (numCols == 0 && tilesetInfo.imageWidth > 0 && tilesetInfo.tileWidth > 0) {
         // Calculate columns based on image width, margin, spacing, tile width
         numCols = (tilesetInfo.imageWidth - 2 * tilesetInfo.margin + tilesetInfo.spacing) / (tilesetInfo.tileWidth + tilesetInfo.spacing);
    }
     if (numCols == 0) numCols = 1; // Avoid division by zero

    unsigned tileX = localId % numCols;
    unsigned tileY = localId / numCols;

    // Calculate pixel coordinates within the tileset image, considering margin and spacing
    SDL_Rect srcRect;
    srcRect.x = tilesetInfo.margin + tileX * (tilesetInfo.tileWidth + tilesetInfo.spacing);
    srcRect.y = tilesetInfo.margin + tileY * (tilesetInfo.tileHeight + tilesetInfo.spacing);
    srcRect.w = tilesetInfo.tileWidth;
    srcRect.h = tilesetInfo.tileHeight;

    return srcRect;
}


} // namespace TuxArena