#ifndef TUXARENA_MAP_H
#define TUXARENA_MAP_H

#include <string>
#include <vector>
#include <cstdint>

namespace TuxArena {

// Forward declarations
class Renderer;

// Basic Tile structure (can be expanded with properties like collision, texture ID, etc.)
struct Tile {
    uint32_t gid; // Global Tile ID (from Tiled)
    // Add more properties as needed, e.g., type, flags
};

class Map {
public:
    Map();
    ~Map();

    // Loads map data from a file (e.g., TMX file)
    bool load(const std::string& filePath);
    void unload();

    bool isLoaded() const { return m_isLoaded; }
    const std::string& getName() const { return m_name; }
    int getWidth() const { return m_width; } // Map width in tiles
    int getHeight() const { return m_height; } // Map height in tiles
    int getTileWidth() const { return m_tileWidth; } // Tile width in pixels
    int getTileHeight() const { return m_tileHeight; } // Tile height in pixels

    // Get tile data for a specific layer and coordinates
    // const Tile& getTile(int layerIndex, int x, int y) const;

private:
    bool m_isLoaded = false;
    std::string m_name;
    int m_width = 0;
    int m_height = 0;
    int m_tileWidth = 0;
    int m_tileHeight = 0;

    // Store map layers (e.g., vector of vectors of Tiles, or flat array)
    // std::vector<std::vector<Tile>> m_layers;
    // Or for simplicity, just a single layer for now:
    // std::vector<Tile> m_tiles; // Flat array for a single layer

    // Add textures/tilesets needed for rendering
    // std::map<uint32_t, Texture*> m_tilesets;
};

} // namespace TuxArena

#endif // TUXARENA_MAP_H
