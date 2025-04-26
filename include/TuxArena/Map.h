// include/TuxArena/Map.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory> // Potentially for storing collision shapes if complex

// Forward declare tmxlite types to avoid including its headers here if possible,
// although including them might be necessary for certain return types/members.
// Let's include the core header as it's central to this class.
#include <tmxlite/Map.hpp>
#include <tmxlite/Layer.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/Tileset.hpp>

// Forward declare SDL types (optional, include if needed)
struct SDL_Texture;
struct SDL_Rect;


// Move MapLayer enum here from Renderer.h as it relates to map structure
enum class MapLayerType {
    Unknown,
    TileLayer,
    ObjectGroup,
    ImageLayer,
    GroupLayer
};

// Structure to hold processed spawn point data
struct SpawnPoint {
    float x = 0.0f;
    float y = 0.0f;
    std::string name; // Optional name from Tiled
    std::string type; // Type property from Tiled (e.g., "PlayerSpawn", "ItemSpawn")
    // Add other relevant properties (e.g., team, initial rotation)
};

// Structure to hold processed collision shape data
// Using a simple polygon representation for now
struct CollisionShape {
    enum class Type { Rectangle, Ellipse, Polygon, Polyline };
    Type type = Type::Polygon;
    // Store points relative to the map origin (or object origin if needed)
    std::vector<std::pair<float, float>> points;
    // Bounding box for quick checks?
    float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
};

// Structure to hold essential information about a tileset needed for rendering
struct TilesetInfo {
    unsigned firstGid = 0;       // First global tile ID in this tileset
    unsigned tileCount = 0;      // Number of tiles in this tileset
    int tileWidth = 0;           // Width of a single tile in pixels
    int tileHeight = 0;          // Height of a single tile in pixels
    int columns = 0;             // Number of tile columns in the tileset image
    std::string imagePath;       // Relative path to the tileset image file
    int imageWidth = 0;          // Width of the tileset image in pixels
    int imageHeight = 0;         // Height of the tileset image in pixels
    int spacing = 0;             // Spacing between tiles in the image
    int margin = 0;              // Margin around tiles in the image
};


namespace TuxArena {

class Renderer; // Forward declare for potential friend class or methods needing it

class MapManager {
public:
    MapManager();
    ~MapManager();

    // Prevent copying
    MapManager(const MapManager&) = delete;
    MapManager& operator=(const MapManager&) = delete;

    /**
     * @brief Loads map data from a TMX file.
     * @param filePath Path to the .tmx map file.
     * @param baseAssetPath Base path to prepend to relative paths in the TMX (e.g., "assets/").
     * @return True if loading and processing were successful, false otherwise.
     */
    bool loadMap(const std::string& filePath, const std::string& baseAssetPath = "");

    /**
     * @brief Unloads the current map data and frees associated resources.
     */
    void unloadMap();

    /**
     * @brief Checks if a map is currently loaded.
     * @return True if a map is loaded, false otherwise.
     */
    bool isMapLoaded() const { return m_mapLoaded; }

    // --- Map Property Accessors ---

    /**
     * @brief Gets the width of the map in tiles.
     * @return Map width in tiles, or 0 if no map is loaded.
     */
    unsigned getMapWidthTiles() const;

    /**
     * @brief Gets the height of the map in tiles.
     * @return Map height in tiles, or 0 if no map is loaded.
     */
    unsigned getMapHeightTiles() const;

    /**
     * @brief Gets the width of a single tile in pixels.
     * @return Tile width in pixels, or 0 if no map is loaded.
     */
    unsigned getTileWidth() const;

     /**
      * @brief Gets the height of a single tile in pixels.
      * @return Tile height in pixels, or 0 if no map is loaded.
      */
    unsigned getTileHeight() const;

    /**
     * @brief Gets the total width of the map in pixels.
     * @return Map width in pixels, or 0 if no map is loaded.
     */
    unsigned getMapWidthPixels() const;

    /**
     * @brief Gets the total height of the map in pixels.
     * @return Map height in pixels, or 0 if no map is loaded.
     */
    unsigned getMapHeightPixels() const;

    // --- Data Accessors ---

    /**
     * @brief Gets all the parsed collision shapes from the map.
     * @return Constant reference to the vector of collision shapes.
     */
    const std::vector<CollisionShape>& getCollisionShapes() const { return m_collisionShapes; }

    /**
     * @brief Gets all the parsed spawn points from the map.
     * @return Constant reference to the vector of spawn points.
     */
    const std::vector<SpawnPoint>& getSpawnPoints() const { return m_spawnPoints; }

    /**
     * @brief Gets the processed information for all tilesets used in the map.
     * @return Constant reference to the map storing tileset info (key: first GID).
     */
    const std::map<unsigned, TilesetInfo>& getTilesets() const { return m_tilesets; }

    /**
     * @brief Gets the raw tmxlite layer data (for advanced use or direct rendering).
     * Use with caution regarding lifetime management.
     * @return Constant reference to the vector of tmxlite layer pointers.
     */
     const std::vector<tmx::Layer::Ptr>& getRawLayers() const;


     /**
     * @brief Finds the TilesetInfo associated with a specific Global Tile ID (GID).
     * @param gid The Global Tile ID.
     * @return Pointer to the TilesetInfo if found, otherwise nullptr.
     */
     const TilesetInfo* findTilesetForGid(unsigned gid) const;

     /**
     * @brief Calculates the source rectangle within a tileset image for a given GID.
     * @param gid The Global Tile ID (must be non-zero).
     * @param tilesetInfo The TilesetInfo associated with the GID.
     * @return SDL_Rect representing the source area in the tileset texture. Returns {0,0,0,0} on error.
     */
     SDL_Rect getSourceRectForGid(unsigned gid, const TilesetInfo& tilesetInfo) const;


private:
    /**
     * @brief Processes layers recursively, extracting collision shapes and spawn points.
     * @param layer The tmxlite layer/group to process.
     */
    void processLayer(const tmx::Layer& layer);

    /**
     * @brief Extracts collision shapes from a tmxlite object group.
     * @param group The tmxlite object group layer.
     */
    void processObjectLayer(const tmx::ObjectGroup& group);

    /**
     * @brief Extracts spawn points from a tmxlite object group.
     * @param group The tmxlite object group layer.
     */
    void processSpawnpointLayer(const tmx::ObjectGroup& group); // Example if spawns are in dedicated layer


    // Internal Data
    tmx::Map m_tmxMap; // The raw parsed map data from tmxlite
    std::string m_mapDirectory; // Directory containing the loaded TMX file
    std::string m_baseAssetPath; // Base path prefixed to relative asset paths

    bool m_mapLoaded = false;

    // Processed Data for easier access
    std::vector<CollisionShape> m_collisionShapes;
    std::vector<SpawnPoint> m_spawnPoints;
    std::map<unsigned, TilesetInfo> m_tilesets; // Keyed by first GID
};

} // namespace TuxArena