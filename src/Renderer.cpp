// src/Renderer.cpp
#include "TuxArena/Renderer.h"
#include "TuxArena/MapManager.h" // Include for renderMap - adjust if map rendering logic changes

// Include SDL headers
#include "SDL3/SDL.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

#include <iostream> // For error logging
#include <utility> // For std::pair used in font cache key

namespace TuxArena {

Renderer::Renderer() {
    // Constructor: Initialize pointers (already done by member initializers)
}

Renderer::~Renderer() {
    // Destructor: Ensure shutdown is called if not already
    if (m_isInitialized) {
        std::cerr << "Warning: Renderer destroyed without calling shutdown() first. Resources might leak if shutdown fails now." << std::endl;
        shutdown(); // Attempt cleanup, but better to call explicitly earlier
    }
}

bool Renderer::initialize(const std::string& windowTitle, int windowWidth, int windowHeight, bool vsync) {
    if (m_isInitialized) {
        std::cerr << "Warning: Renderer::initialize called multiple times." << std::endl;
        return true;
    }

    // --- Initialize SDL Video (already done in main, but double-check?) ---
    // It's generally safe to call SDL_InitSubSystem again if needed,
    // but assuming it was done in main.cpp is okay.
    // if (!SDL_WasInit(SDL_INIT_VIDEO)) {
    //     if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    //          std::cerr << "Failed to init SDL Video Subsystem: " << SDL_GetError() << std::endl;
    //          return false;
    //      }
    // }


    // --- Initialize SDL_image ---
    int imgFlags = IMG_INIT_PNG; // Initialize PNG loading
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        // No need to SDL_QuitSubSystem video here, as it might be used elsewhere
        return false;
    }
    std::cout << "SDL_image initialized successfully." << std::endl;


    // --- Initialize SDL_ttf ---
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        IMG_Quit(); // Clean up SDL_image
        return false;
    }
     std::cout << "SDL_ttf initialized successfully." << std::endl;


    // --- Create Window ---
    m_sdlWindow = SDL_CreateWindow(
        windowTitle.c_str(),
        windowWidth,
        windowHeight,
        SDL_WINDOW_RESIZABLE // Example flag, add others as needed (SDL_WINDOW_HIGH_PIXEL_DENSITY, etc)
    );

    if (!m_sdlWindow) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        IMG_Quit();
        return false;
    }
    std::cout << "SDL Window created successfully." << std::endl;


    // --- Create Renderer ---
    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    if (vsync) {
        rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
         std::cout << "VSync enabled for renderer." << std::endl;
    } else {
         std::cout << "VSync disabled for renderer." << std::endl;
    }

    m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, nullptr, rendererFlags); // Use default graphics driver

    if (!m_sdlRenderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(m_sdlWindow);
        m_sdlWindow = nullptr;
        TTF_Quit();
        IMG_Quit();
        return false;
    }
     std::cout << "SDL Renderer created successfully." << std::endl;

    // Set default draw color for clearing
    SDL_SetRenderDrawColor(m_sdlRenderer, m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);

    m_isInitialized = true;
    std::cout << "Renderer initialization complete." << std::endl;
    return true;
}

void Renderer::shutdown() {
    if (!m_isInitialized) {
        return; // Nothing to shut down
    }
    std::cout << "Shutting down Renderer..." << std::endl;

    // 1. Clear Caches and Destroy Resources
    std::cout << "Clearing texture cache (" << m_textureCache.size() << " items)..." << std::endl;
    for (auto const& [key, val] : m_textureCache) {
        if (val) {
            SDL_DestroyTexture(val);
        }
    }
    m_textureCache.clear();

     std::cout << "Clearing font cache (" << m_fontCache.size() << " items)..." << std::endl;
     for (auto const& [key, val] : m_fontCache) {
         if (val) {
             TTF_CloseFont(val);
         }
     }
     m_fontCache.clear();


    // 2. Destroy Renderer and Window
    if (m_sdlRenderer) {
         std::cout << "Destroying SDL Renderer..." << std::endl;
        SDL_DestroyRenderer(m_sdlRenderer);
        m_sdlRenderer = nullptr;
    }
    if (m_sdlWindow) {
         std::cout << "Destroying SDL Window..." << std::endl;
        SDL_DestroyWindow(m_sdlWindow);
        m_sdlWindow = nullptr;
    }

    // 3. Quit SDL Subsystems Initialized Here
     std::cout << "Shutting down SDL_ttf..." << std::endl;
    TTF_Quit();
     std::cout << "Shutting down SDL_image..." << std::endl;
    IMG_Quit();
    // Do NOT call SDL_Quit() here, as it's handled in main.cpp

    m_isInitialized = false;
    std::cout << "Renderer shutdown complete." << std::endl;
}

void Renderer::clear() {
    if (!m_sdlRenderer) return;
    // Set draw color just in case it was changed
    SDL_SetRenderDrawColor(m_sdlRenderer, m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    SDL_RenderClear(m_sdlRenderer);
}

void Renderer::present() {
    if (!m_sdlRenderer) return;
    SDL_RenderPresent(m_sdlRenderer);
}

SDL_Texture* Renderer::loadTexture(const std::string& filePath) {
    if (!m_sdlRenderer) return nullptr;

    // Check cache first
    auto it = m_textureCache.find(filePath);
    if (it != m_textureCache.end()) {
        return it->second; // Return cached texture
    }

    // Not in cache, try to load
    SDL_Texture* newTexture = IMG_LoadTexture(m_sdlRenderer, filePath.c_str());

    if (!newTexture) {
        std::cerr << "Failed to load texture '" << filePath << "'! IMG_Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }

     // std::cout << "Loaded texture: " << filePath << std::endl; // Debug logging
    // Add to cache
    m_textureCache[filePath] = newTexture;
    return newTexture;
}

void Renderer::drawTexture(SDL_Texture* texture, const SDL_Rect* srcRect, const SDL_FRect* dstRect, double angle, SDL_RendererFlip flip) {
    if (!m_sdlRenderer || !texture) return;

    // SDL_RenderTexture expects SDL_FRect for destination, SDL_Rect for source
     if (SDL_RenderTextureRotated(m_sdlRenderer, texture, srcRect, dstRect, angle, nullptr, flip) != 0) {
         // Use SDL_RenderTexture if no rotation/flip needed? Could be slightly faster.
         // if (angle == 0.0 && flip == SDL_FLIP_NONE) {
         //     SDL_RenderTexture(m_sdlRenderer, texture, srcRect, dstRect);
         // } else { ... } // Needs conversion SDL_FRect center point if using SDL_RenderTextureRotated's center arg
         std::cerr << "Error rendering texture: " << SDL_GetError() << std::endl;
     }
}

void Renderer::drawRect(const SDL_FRect* rect, const SDL_Color& color) {
    if (!m_sdlRenderer || !rect) return;

    SDL_SetRenderDrawColor(m_sdlRenderer, color.r, color.g, color.b, color.a);
    // Enable blend mode for alpha transparency if needed
    // SDL_SetRenderDrawBlendMode(m_sdlRenderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(m_sdlRenderer, rect);
    // Restore default draw color? Optional, depends on usage patterns.
    // SDL_SetRenderDrawColor(m_sdlRenderer, m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
}

void Renderer::drawRectOutline(const SDL_FRect* rect, const SDL_Color& color) {
     if (!m_sdlRenderer || !rect) return;

     SDL_SetRenderDrawColor(m_sdlRenderer, color.r, color.g, color.b, color.a);
     SDL_RenderRect(m_sdlRenderer, rect);
     // Restore default draw color?
}


TTF_Font* Renderer::loadFont(const std::string& filePath, int pointSize) {
     if (!m_isInitialized) return nullptr; // TTF_Init needs to have run

     // Create cache key
     std::pair<std::string, int> fontKey = {filePath, pointSize};

     // Check cache first
     auto it = m_fontCache.find(fontKey);
     if (it != m_fontCache.end()) {
         return it->second; // Return cached font
     }

     // Not in cache, try to load
     TTF_Font* newFont = TTF_OpenFont(filePath.c_str(), pointSize);

     if (!newFont) {
         std::cerr << "Failed to load font '" << filePath << "' at size " << pointSize << "! TTF_Error: " << TTF_GetError() << std::endl;
         return nullptr;
     }

      // std::cout << "Loaded font: " << filePath << " Size: " << pointSize << std::endl; // Debug logging
     // Add to cache
     m_fontCache[fontKey] = newFont;
     return newFont;
}


bool Renderer::drawText(const std::string& text, float x, float y, const std::string& fontPath, int pointSize, const SDL_Color& color) {
     if (!m_sdlRenderer || text.empty()) return false;

     TTF_Font* font = loadFont(fontPath, pointSize);
     if (!font) {
         return false; // Font loading failed
     }

     // 1. Render text surface (using Blended for quality with alpha)
     // Note: TTF_RenderText_Solid is faster but no alpha. UTF8 variants exist too.
     SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
     if (!textSurface) {
         std::cerr << "Failed to render text surface! TTF_Error: " << TTF_GetError() << std::endl;
         // Font is cached, no need to close it here unless loadFont failed internally
         return false;
     }

     // 2. Create texture from surface
     SDL_Texture* textTexture = SDL_CreateTextureFromSurface(m_sdlRenderer, textSurface);
     if (!textTexture) {
          std::cerr << "Failed to create text texture! SDL_Error: " << SDL_GetError() << std::endl;
          SDL_DestroySurface(textSurface); // Clean up surface
          return false;
     }

     // 3. Get texture dimensions and prepare destination rect
     SDL_FRect dstRect = {x, y, static_cast<float>(textSurface->w), static_cast<float>(textSurface->h)};

     // 4. Render the texture
     drawTexture(textTexture, nullptr, &dstRect); // Draw whole texture

     // 5. Clean up surface and texture (texture is temporary for this draw call)
     SDL_DestroySurface(textSurface);
     SDL_DestroyTexture(textTexture);

     return true;
}


// Placeholder for Map Rendering - requires MapManager implementation details
void Renderer::renderMap(const MapManager& mapManager, MapLayer layer) {
     if (!m_isInitialized || !m_sdlRenderer) return;

     // --- This is highly dependent on how MapManager stores map data ---
     // Example Pseudo-code:
     // 1. Get Tile Layers for the specified MapLayer (Background/Foreground) from mapManager
     // 2. For each layer:
     //    a. Get the Tileset Texture associated with this layer (via loadTexture)
     //    b. Iterate through the tile grid data for the layer (from mapManager)
     //    c. For each non-empty tile GID (Global ID):
     //       i. Calculate the source SDL_Rect within the tileset texture based on GID.
     //      ii. Calculate the destination SDL_FRect on the screen based on grid position.
     //     iii. Apply camera offset/zoom if implemented.
     //      iv. Call drawTexture(tilesetTexture, &sourceRect, &destRect);

     // Example drawing a placeholder rectangle instead:
     /*
     SDL_FRect mapBounds = { 50.0f, 50.0f, 600.0f, 400.0f }; // Example bounds
     SDL_Color mapColor = (layer == MapLayer::Background) ?
                           SDL_Color{50, 50, 50, 255} : // Dark grey for background
                           SDL_Color{100, 100, 100, 150}; // Lighter grey for foreground

     drawRect(&mapBounds, mapColor);
     drawText( (layer == MapLayer::Background) ? "Map Background Layer (Placeholder)" : "Map Foreground Layer (Placeholder)",
               mapBounds.x + 10, mapBounds.y + 10, "assets/fonts/nokia.ttf", 16, {255, 255, 255, 255});
     */

     // A real implementation needs access to mapManager.getTileLayers(), mapManager.getTilesetTexture(), etc.
      std::cerr << "Warning: renderMap() called but not fully implemented yet." << std::endl;

}


} // namespace TuxArena