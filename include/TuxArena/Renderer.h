// include/TuxArena/Renderer.h
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map> // For texture and font caching

// Forward declare SDL types to reduce header dependencies slightly, though
// including SDL headers directly is often necessary for function signatures.
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
typedef struct _TTF_Font TTF_Font; // From SDL_ttf
struct SDL_Color;
struct SDL_FRect; // Use floating-point rects for potentially smoother rendering
struct SDL_Rect;

// Forward declare internal types if needed
namespace TuxArena {
    class MapManager; // For renderMap method signature
    enum class MapLayer;  // Define elsewhere, maybe Map.h? For now, assume its definition.
}

// Define MapLayer enum here for now, move to Map.h later
enum class MapLayer {
    Background,
    Foreground
};


namespace TuxArena {

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Prevent copying
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * @brief Initializes the SDL video subsystem, creates a window and renderer.
     * @param windowTitle The title for the application window.
     * @param windowWidth The desired width of the window in pixels.
     * @param windowHeight The desired height of the window in pixels.
     * @param vsync Enable vertical synchronization.
     * @return True if initialization was successful, false otherwise.
     */
    bool initialize(const std::string& windowTitle, int windowWidth, int windowHeight, bool vsync);

    /**
     * @brief Shuts down the renderer, destroys the window, and cleans up resources.
     */
    void shutdown();

    /**
     * @brief Clears the render target with the default background color.
     */
    void clear();

    /**
     * @brief Presents the rendered frame to the screen.
     */
    void present();

    /**
     * @brief Loads a texture from a file path. Caches textures to avoid reloading.
     * @param filePath Path to the image file.
     * @return Pointer to the loaded SDL_Texture, or nullptr on failure.
     */
    SDL_Texture* loadTexture(const std::string& filePath);

    /**
     * @brief Draws a texture to the screen at a specific position and scale.
     * @param texture The texture to draw.
     * @param srcRect The source rectangle within the texture (or nullptr for the whole texture).
     * @param dstRect The destination rectangle on the screen (position and size).
     * @param angle The rotation angle in degrees (clockwise).
     * @param flip SDL_RendererFlip flags (SDL_FLIP_NONE, SDL_FLIP_HORIZONTAL, SDL_FLIP_VERTICAL).
     */
    void drawTexture(SDL_Texture* texture, const SDL_Rect* srcRect, const SDL_FRect* dstRect, double angle = 0.0, SDL_RendererFlip flip = SDL_FLIP_NONE);

     /**
     * @brief Draws a filled rectangle.
     * @param rect The destination rectangle (position and size).
     * @param color The color of the rectangle.
     */
    void drawRect(const SDL_FRect* rect, const SDL_Color& color);

     /**
     * @brief Draws outlined rectangle.
     * @param rect The destination rectangle (position and size).
     * @param color The color of the rectangle outline.
     */
    void drawRectOutline(const SDL_FRect* rect, const SDL_Color& color);


    /**
     * @brief Loads a TTF font from a file path. Caches fonts.
     * @param filePath Path to the TTF font file.
     * @param pointSize The desired font size in points.
     * @return Pointer to the loaded TTF_Font, or nullptr on failure.
     */
    TTF_Font* loadFont(const std::string& filePath, int pointSize);

    /**
     * @brief Renders text to the screen using a loaded font.
     * @param text The string to render.
     * @param x The x-coordinate for the top-left corner of the text.
     * @param y The y-coordinate for the top-left corner of the text.
     * @param fontPath The path to the font file to use.
     * @param pointSize The size of the font.
     * @param color The color of the text.
     * @return True if rendering succeeded, false otherwise.
     */
    bool drawText(const std::string& text, float x, float y, const std::string& fontPath, int pointSize, const SDL_Color& color);


    /**
     * @brief Renders the specified layers of a map.
     * @param mapManager Reference to the map manager containing map data.
     * @param layer The layer(s) to render (e.g., Background, Foreground).
     */
    void renderMap(const MapManager& mapManager, MapLayer layer); // Implementation depends heavily on MapManager structure


    /**
    * @brief Get the underlying SDL_Renderer pointer. Use with caution.
    * @return Raw pointer to the SDL_Renderer instance.
    */
    SDL_Renderer* getSDLRenderer() const { return m_sdlRenderer; }


    // Potential additions:
    // - drawLine, drawCircle
    // - setDrawColor
    // - getWindowSize
    // - Camera/View manipulation functions

private:
    SDL_Window* m_sdlWindow = nullptr;
    SDL_Renderer* m_sdlRenderer = nullptr;

    // Resource Caches
    std::map<std::string, SDL_Texture*> m_textureCache;
    // Cache fonts based on path AND size
    std::map<std::pair<std::string, int>, TTF_Font*> m_fontCache;

    // Default background color (e.g., black)
    SDL_Color m_clearColor = {0, 0, 0, 255};

    // Flag to track initialization status
    bool m_isInitialized = false;
};

} // namespace TuxArena