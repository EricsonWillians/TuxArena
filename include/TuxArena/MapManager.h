#ifndef TUXARENA_MAPMANAGER_H
#define TUXARENA_MAPMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <tmxlite/Map.hpp>
#include "tmxlite/TileLayer.hpp"
#include "tmxlite/ObjectGroup.hpp"
#include "tmxlite/ImageLayer.hpp"
#include "tmxlite/LayerGroup.hpp"
#include "SDL2/SDL_rect.h" // For SDL_Rect
#include "TuxArena/Entity.h" // For Vec2

namespace TuxArena {

enum class MapLayer {
    Background,
    Foreground,
    Collision,
    Objects
};

// Struct to hold tileset information for rendering
struct TilesetInfo {
    unsigned firstGid = 0;
    unsigned tileCount = 0;
    unsigned tileWidth = 0;
    unsigned tileHeight = 0;
    unsigned columns = 0;
    unsigned spacing = 0;
    unsigned margin = 0;
    unsigned imageWidth = 0;
    unsigned imageHeight = 0;
    std::string imagePath; // Absolute path to the tileset image
};

// Struct to hold collision shape information
struct CollisionShape {
    enum class Type {
        Rectangle,
        Ellipse,
        Polygon,
        Polyline
    };
    Type type;
    float minX, minY, maxX, maxY; // Bounding box
    std::vector<Vec2> points; // For polygons/polylines

    // Constructor for convenience
    CollisionShape(Type t, float x1, float y1, float x2, float y2) :
        type(t), minX(x1), minY(y1), maxX(x2), maxY(y2) {}
};

// Struct to hold spawn point information
struct SpawnPoint {
    float x, y;
    std::string name;
    std::string type;
};

class MapManager {
public:
    MapManager();
    ~MapManager();

    bool loadMap(const std::string& filePath);
    void unloadMap();
    bool isMapLoaded() const { return m_isMapLoaded; }
    std::string getMapName() const { return m_mapName; }

    // Accessors for map properties
    unsigned getMapWidthTiles() const;
    unsigned getMapHeightTiles() const;
    unsigned getTileWidth() const;
    unsigned getTileHeight() const;
    unsigned getMapWidthPixels() const;
    unsigned getMapHeightPixels() const;

    // Accessors for map data (for rendering and collision)
    const std::vector<tmx::Layer::Ptr>& getRawLayers() const;
    const TilesetInfo* findTilesetForGid(unsigned gid) const;
    SDL_Rect getSourceRectForGid(unsigned gid, const TilesetInfo& tilesetInfo) const;

    const std::vector<CollisionShape>& getCollisionShapes() const { return m_collisionShapes; }
    const std::vector<SpawnPoint>& getSpawnPoints() const { return m_spawnPoints; }

private:
    bool m_isMapLoaded = false;
    std::string m_mapName;
    std::string m_mapDirectory; // Directory where the TMX map file is located

    tmx::Map m_tmxMap; // The loaded TMX map object

    unsigned int m_mapWidth = 0;
    unsigned int m_mapHeight = 0;
    unsigned int m_tileWidth = 0;
    unsigned int m_tileHeight = 0;

    // Cached tileset information (key is firstGid)
    std::map<unsigned, TilesetInfo> m_tilesets;

    std::vector<CollisionShape> m_collisionShapes;
    std::vector<SpawnPoint> m_spawnPoints;

    // Fallback map data
    bool m_useFallbackMap = false;
    void createFallbackMap();

    // Helper to process layers recursively
    void processLayer(const tmx::Layer& layer);
    void processObjectLayer(const tmx::ObjectGroup& group);
};

} // namespace TuxArena

#endif // TUXARENA_MAPMANAGER_H