# Map Creation with Tiled

This document outlines the basic steps to create maps for TuxArena using the Tiled Map Editor.

## 1. Install Tiled Map Editor

If you don't have Tiled installed, you can download it from the official website:

[https://www.mapeditor.org/](https://www.mapeditor.org/)

## 2. Create a New Map

1.  Open Tiled and go to `File > New > New Map...`.
2.  **Orientation:** Choose `Orthogonal`.
3.  **Tile Layer Format:** `CSV` (Comma Separated Values) is generally efficient.
4.  **Tile Render Order:** `Right Down` is a common choice.
5.  **Map Size:**
    *   **Width & Height:** Set the number of tiles horizontally and vertically (e.g., 32x24).
    *   **Tile Width & Height:** Set the size of each tile in pixels (e.g., 32x32).
6.  Click `Save As...` and save your map as a `.tmx` file in the `maps/` directory of the TuxArena project (e.g., `maps/my_arena.tmx`).

## 3. Create a Tileset

Tilesets are collections of images that make up your map's visual elements.

1.  Go to `Map > New Tileset...`.
2.  **Name:** Give your tileset a descriptive name (e.g., `topdown_tileset`).
3.  **Type:** `Collection of Images` or `Image`.
    *   **Image:** If you have a single image file containing all your tiles (a spritesheet).
        *   **Source:** Browse to your tileset image file (e.g., `assets/tilesets/topdown_tileset.png`).
        *   **Tile Width & Height:** Ensure these match the tile size you set for your map.
        *   **Margin & Spacing:** If your tileset image has padding around or between tiles, specify these values.
    *   **Collection of Images:** If each tile is a separate image file.
        *   You'll add individual images later.
4.  Click `Save As...` and save your tileset as a `.tsx` file in the same directory as your `.tmx` map file (e.g., `maps/topdown_tileset.tsx`).
5.  **Embed in map:** Ensure the tileset is embedded in the map. This makes the `.tmx` file self-contained.

## 4. Add Layers

Maps in Tiled are composed of layers. TuxArena expects at least a background layer and an object layer for collision and spawn points.

1.  **Tile Layer (Background/Foreground):**
    *   Go to `Layer > Add Tile Layer...`.
    *   Name it appropriately (e.g., `Background`, `Foreground`).
    *   Use the `Stamp` tool to draw your map's visual elements using tiles from your tileset.
2.  **Object Layer (Collision & Spawn Points):**
    *   Go to `Layer > Add Object Layer...`.
    *   Name it `Collision` for collision objects or `SpawnPoints` for spawn points.
    *   **Collision Objects:**
        *   Select the `Insert Rectangle` tool.
        *   Draw rectangles over areas where entities should collide (e.g., walls, obstacles).
        *   You can add custom properties to objects (e.g., `type: wall`, `damage: 10`).
    *   **Spawn Points:**
        *   Select the `Insert Point` tool.
        *   Place points where players or other entities should spawn.
        *   Add custom properties:
            *   `name`: A unique identifier for the spawn point (e.g., `player_start_1`).
            *   `type`: The type of entity that should spawn here (e.g., `player`, `enemy`).

## 5. Save Your Map

Regularly save your `.tmx` file (`File > Save`).

## 6. Integrate into TuxArena

To use your new map in TuxArena, you'll need to specify it when running the game. You can do this via command-line arguments:

```bash
./build/bin/tuxarena --map maps/my_arena.tmx
```

Replace `maps/my_arena.tmx` with the actual path to your map file.

## Important Notes:

*   **Asset Paths:** Ensure that the paths to your tileset images within the `.tmx` file are correct relative to the `.tmx` file itself, or absolute paths if necessary.
*   **Layer Names:** While TuxArena can process various layers, using consistent naming like `Collision` and `SpawnPoints` for object layers will help with automatic detection and processing.
*   **Collision Shapes:** For simple AABB collision, rectangles are sufficient. For more complex shapes, Tiled supports polygons and polylines.
*   **Properties:** Custom properties in Tiled can be used to add metadata to tiles, objects, and layers, which can be read and utilized by the game engine.
