#ifndef TUXARENA_RENDERER_H
#define TUXARENA_RENDERER_H

#include <string>
#include <map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_render.h> // For SDL_FPoint
#include <SDL_opengl.h> // For GLuint
#include "TuxArena/MapManager.h" // For MapLayer

namespace TuxArena {

// Struct for colors
struct Color {
    uint8_t r, g, b, a;
};

#include "TuxArena/AssetManager.h"

class AssetManager;

class Renderer {
public:
    Renderer(AssetManager* assetManager);
    ~Renderer();

    bool initialize(const std::string& title, int width, int height, bool vsync);
    void shutdown();

    void clear();
    void present();

    // Texture management
    SDL_Texture* loadTexture(const std::string& filePath);
    void destroyTexture(SDL_Texture* texture);

    // Drawing functions
    void drawTexture(SDL_Texture* texture, const SDL_Rect* srcRect, const SDL_FRect* dstRect, double angle = 0.0, const SDL_FPoint* center = nullptr, SDL_RendererFlip flip = SDL_FLIP_NONE);
    void drawRect(const SDL_FRect* rect, const Color& color, bool filled = false);
    void drawLine(float x1, float y1, float x2, float y2, const Color& color);
    void drawCircle(float x, float y, float radius, const Color& color, bool filled = false);
    void drawText(const std::string& text, float x, float y, const std::string& fontPath, int fontSize, const Color& color);

    // Map rendering
    void renderMap(const MapManager& mapManager, MapLayer layer);

    // Getters
    SDL_Window* getSDLWindow() const { return m_sdlWindow; }
    SDL_Renderer* getSDLRenderer() const { return m_sdlRenderer; }
    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    GLuint getOpenGLTextureID(SDL_Texture* texture);

private:
    SDL_Window* m_sdlWindow = nullptr;
    SDL_Renderer* m_sdlRenderer = nullptr;
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    bool m_isInitialized = false; // Added missing member
    Color m_clearColor = {20, 20, 30, 255}; // Darker shade for gothic theme

    // Cache for loaded textures and fonts
    std::map<std::string, SDL_Texture*> m_textureCache;
    std::map<std::pair<std::string, int>, TTF_Font*> m_fontCache;

    TTF_Font* getFont(const std::string& fontPath, int fontSize);
    SDL_Texture* generatePlaceholderTexture(int width, int height, const std::string& assetName);
};

} // namespace TuxArena

#endif // TUXARENA_RENDERER_H
