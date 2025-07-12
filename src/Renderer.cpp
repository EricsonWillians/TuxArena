// src/Renderer.cpp
#include "TuxArena/Renderer.h"
#include "TuxArena/Log.h"
#include "TuxArena/UI.h"
#include "../include/TuxArena/MapManager.h" // Include for renderMap - adjust if map rendering logic changes

// Include ImGui
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

// Include SDL headers
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"

#include <iostream> // For error logging
#include <utility> // For std::pair used in font cache key

namespace TuxArena {

Renderer::Renderer(AssetManager* assetManager) : m_assetManager(assetManager) {
    // Log::Info("Renderer created.");
}

Renderer::~Renderer() {
    // Destructor: Ensure shutdown is called if not already
    if (m_isInitialized) {
        Log::Warning("Renderer destroyed without calling shutdown() first. Resources might leak if shutdown fails now.");
        shutdown(); // Attempt cleanup, but better to call explicitly earlier
    }
}

bool Renderer::initialize(const std::string& windowTitle, int windowWidth, int windowHeight, bool vsync) {
    if (m_isInitialized) {
        Log::Warning("Renderer::initialize called multiple times.");
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
        Log::Error("SDL_image could not initialize! IMG_Error: " + std::string(IMG_GetError()));
        // No need to SDL_QuitSubSystem video here, as it might be used elsewhere
        return false;
    }
    Log::Info("SDL_image initialized successfully.");


    // --- Initialize SDL_ttf ---
    if (TTF_Init() == -1) {
        Log::Error("SDL_ttf could not initialize! TTF_Error: " + std::string(TTF_GetError()));
        IMG_Quit(); // Clean up SDL_image
        return false;
    }
     Log::Info("SDL_ttf initialized successfully.");


    // --- Create Window ---
    m_sdlWindow = SDL_CreateWindow(
        windowTitle.c_str(),
        SDL_WINDOWPOS_UNDEFINED, // x position
        SDL_WINDOWPOS_UNDEFINED, // y position
        windowWidth,
        windowHeight,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL // Enable OpenGL context for ImGui
    );

    if (!m_sdlWindow) {
        Log::Error("Window could not be created! SDL_Error: " + std::string(SDL_GetError()));
        TTF_Quit();
        IMG_Quit();
        return false;
    }
    Log::Info("SDL Window created successfully.");


    // --- Create Renderer ---
    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    if (vsync) {
        rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
        Log::Info("VSync enabled for renderer.");
    } else {
        Log::Info("VSync disabled for renderer.");
    }

    m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, rendererFlags);

    if (!m_sdlRenderer) {
        Log::Error("Renderer could not be created! SDL_Error: " + std::string(SDL_GetError()));
        SDL_DestroyWindow(m_sdlWindow);
        m_sdlWindow = nullptr;
        TTF_Quit();
        IMG_Quit();
        return false;
    }
    Log::Info("SDL Renderer created successfully.");

    // Set default draw color for clearing
    SDL_SetRenderDrawColor(m_sdlRenderer, m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(m_sdlWindow, SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init("#version 130");

    // Set the theme
    TuxArena::UI::SetGothicTheme();

    m_isInitialized = true;
    Log::Info("Renderer initialization complete.");
    return true;
}

void Renderer::shutdown() {
    if (!m_isInitialized) {
        return; // Nothing to shut down
    }
    Log::Info("Shutting down Renderer...");

    // Shutdown ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // 1. Clear Caches and Destroy Resources
    Log::Info("Clearing texture cache (" + std::to_string(m_textureCache.size()) + " items)...");
    for (auto const& [key, val] : m_textureCache) {
        if (val) {
            SDL_DestroyTexture(val);
        }
    }
    m_textureCache.clear();

     Log::Info("Clearing font cache (" + std::to_string(m_fontCache.size()) + " items)...");
     for (auto const& [key, val] : m_fontCache) {
         if (val) {
             TTF_CloseFont(val);
         }
     }
     m_fontCache.clear();


    // 2. Destroy Renderer and Window
    if (m_sdlRenderer) {
        Log::Info("Destroying SDL Renderer...");
        SDL_DestroyRenderer(m_sdlRenderer);
        m_sdlRenderer = nullptr;
    }
    if (m_sdlWindow) {
        Log::Info("Destroying SDL Window...");
        SDL_DestroyWindow(m_sdlWindow);
        m_sdlWindow = nullptr;
    }

    // 3. Quit SDL Subsystems Initialized Here
     Log::Info("Shutting down SDL_ttf...");
    TTF_Quit();
    Log::Info("Shutting down SDL_image...");
    IMG_Quit();
    // Do NOT call SDL_Quit() here, as it's handled in main.cpp

    m_isInitialized = false;
    Log::Info("Renderer shutdown complete.");
}

void Renderer::clear() {
    if (!m_sdlRenderer) return;
    // Set draw color just in case it was changed
    SDL_SetRenderDrawColor(m_sdlRenderer, m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    SDL_RenderClear(m_sdlRenderer);
}

void Renderer::present() {
    if (!m_sdlWindow) return; // Ensure window exists
    SDL_GL_SwapWindow(m_sdlWindow);
}

SDL_Texture* Renderer::loadTexture(const std::string& relativePath) {
    std::string fullPath = m_assetManager->resolvePath(relativePath);
    if (fullPath.empty()) {
        Log::Warning("Texture not found: " + relativePath);
        return createPlaceholderTexture();
    }

    SDL_Texture* texture = IMG_LoadTexture(m_renderer, fullPath.c_str());
    if (!texture) {
        Log::Warning("Failed to load texture: " + fullPath + ", error: " + IMG_GetError());
        return createPlaceholderTexture();
    }

    return texture;
}
    if (!m_sdlRenderer) return nullptr;

    // Check cache first
    auto it = m_textureCache.find(filePath);
    if (it != m_textureCache.end()) {
        return it->second; // Return cached texture
    }

    // Not in cache, try to load
    SDL_Texture* newTexture = IMG_LoadTexture(m_sdlRenderer, filePath.c_str());

    if (!newTexture) {
        Log::Error("Failed to load texture '" + filePath + "'! IMG_Error: " + std::string(IMG_GetError()) + ". Generating placeholder.");
        // If loading fails, generate a placeholder texture
        newTexture = generatePlaceholderTexture(64, 64, filePath);
        if (!newTexture) {
            Log::Error("Failed to generate placeholder texture for '" + filePath + "'.");
            return nullptr;
        }
    }

     // std::cout << "Loaded texture: " << filePath << std::endl; // Debug logging
    // Add to cache
    m_textureCache[filePath] = newTexture;
    return newTexture;
}

void Renderer::drawTexture(SDL_Texture* texture, const SDL_Rect* srcRect, const SDL_FRect* dstRect, double angle, const SDL_FPoint* center, SDL_RendererFlip flip) {
    if (!m_sdlRenderer || !texture) return;

    // SDL2's SDL_RenderCopyEx takes SDL_Rect for src and SDL_Rect for dst
    // We need to convert SDL_FRect to SDL_Rect for dstRect if it's used.
    // For now, assuming dstRect is already in integer coordinates or will be converted.
    // If dstRect is float, it needs to be cast to SDL_Rect or use SDL_RenderCopyExF if available (SDL 2.0.10+)

    if (SDL_RenderCopyExF(m_sdlRenderer, texture, srcRect, dstRect, angle, center, flip) != 0) {
        Log::Error("Error rendering texture: " + std::string(SDL_GetError()));
    }
}

void Renderer::destroyTexture(SDL_Texture* texture) {
    if (!texture) return;

    // Remove from cache if present
    for (auto it = m_textureCache.begin(); it != m_textureCache.end(); ++it) {
        if (it->second == texture) {
            m_textureCache.erase(it);
            break;
        }
    }
    SDL_DestroyTexture(texture);
}

void Renderer::drawRect(const SDL_FRect* rect, const Color& color, bool filled) {
    if (!m_sdlRenderer || !rect) return;

    SDL_SetRenderDrawColor(m_sdlRenderer, color.r, color.g, color.b, color.a);
    if (filled) {
        SDL_RenderFillRectF(m_sdlRenderer, rect);
    } else {
        SDL_RenderDrawRectF(m_sdlRenderer, rect);
    }
}

void Renderer::drawLine(float x1, float y1, float x2, float y2, const Color& color) {
    if (!m_sdlRenderer) return;
    SDL_SetRenderDrawColor(m_sdlRenderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLineF(m_sdlRenderer, x1, y1, x2, y2);
}


TTF_Font* Renderer::getFont(const std::string& relativePath, int fontSize) {
    std::string fullPath = m_assetManager->resolvePath(relativePath);
    if (fullPath.empty()) {
        Log::Warning("Font not found: " + relativePath);
        return getFont("assets/fonts/nokia.ttf", fontSize); // Fallback to default font
    }

    std::string key = fullPath + std::to_string(fontSize);
    if (m_fontCache.count(key)) {
        return m_fontCache[key];
    }

    TTF_Font* font = TTF_OpenFont(fullPath.c_str(), fontSize);
    if (!font) {
        Log::Warning("Failed to load font: " + fullPath + ", error: " + TTF_GetError());
        return getFont("assets/fonts/nokia.ttf", fontSize); // Fallback to default font
    }

    m_fontCache[key] = font;
    return font;
}

SDL_Texture* Renderer::createPlaceholderTexture() {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, 32, 32, 32, 0, 0, 0, 0);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 0, 255));
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}


void Renderer::drawText(const std::string& text, float x, float y, const std::string& fontPath, int fontSize, const Color& color) {
     if (!m_sdlRenderer || text.empty()) return;

     TTF_Font* font = getFont(fontPath, fontSize);
     if (!font) {
         return; // Font loading failed
     }

     // 1. Render text surface (using Blended for quality with alpha)
     // Note: TTF_RenderText_Solid is faster but no alpha. UTF8 variants exist too.
     SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), {color.r, color.g, color.b, color.a});
     if (!textSurface) {
         Log::Error("Failed to render text surface! TTF_Error: " + std::string(TTF_GetError()));
         // Font is cached, no need to close it here unless loadFont failed internally
         return;
     }

     // 2. Create texture from surface
     SDL_Texture* textTexture = SDL_CreateTextureFromSurface(m_sdlRenderer, textSurface);
     if (!textTexture) {
          Log::Error("Failed to create text texture! SDL_Error: " + std::string(SDL_GetError()));
          SDL_FreeSurface(textSurface); // Clean up surface
          return;
     }

     // 3. Get texture dimensions and prepare destination rect
     SDL_FRect dstRect = {x, y, static_cast<float>(textSurface->w), static_cast<float>(textSurface->h)};

     // 4. Render the texture
     drawTexture(textTexture, nullptr, &dstRect); // Draw whole texture

     // 5. Clean up surface and texture (texture is temporary for this draw call)
     SDL_FreeSurface(textSurface);
     SDL_DestroyTexture(textTexture);
}


void Renderer::drawCircle(float x, float y, float radius, const Color& color, bool filled) {
    if (!m_sdlRenderer) return;

    SDL_SetRenderDrawColor(m_sdlRenderer, color.r, color.g, color.b, color.a);

    if (filled) {
        // Draw filled circle using a series of horizontal lines
        for (int dy = 0; dy <= radius; ++dy) {
            float dx = std::sqrt(radius * radius - dy * dy);
            SDL_RenderDrawLineF(m_sdlRenderer, x - dx, y + dy, x + dx, y + dy);
            SDL_RenderDrawLineF(m_sdlRenderer, x - dx, y - dy, x + dx, y - dy);
        }
    } else {
        // Draw outline using midpoint circle algorithm or similar
        int segments = 36; // Number of segments to approximate the circle
        for (int i = 0; i < segments; ++i) {
            float angle1 = (float)i / segments * 2 * M_PI;
            float angle2 = (float)(i + 1) / segments * 2 * M_PI;
            float x1 = x + radius * std::cos(angle1);
            float y1 = y + radius * std::sin(angle1);
            float x2 = x + radius * std::cos(angle2);
            float y2 = y + radius * std::sin(angle2);
            SDL_RenderDrawLineF(m_sdlRenderer, x1, y1, x2, y2);
        }
    }
}

SDL_Texture* Renderer::generatePlaceholderTexture(int width, int height, const std::string& assetName) {
    if (!m_sdlRenderer) return nullptr;

    // Create a new blank texture
    SDL_Texture* placeholderTexture = SDL_CreateTexture(m_sdlRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    if (!placeholderTexture) {
        Log::Error("Failed to create placeholder texture: " + std::string(SDL_GetError()));
        return nullptr;
    }

    // Set the texture as the current rendering target
    SDL_SetRenderTarget(m_sdlRenderer, placeholderTexture);

    // Save current draw color
    Uint8 oldR, oldG, oldB, oldA;
    SDL_GetRenderDrawColor(m_sdlRenderer, &oldR, &oldG, &oldB, &oldA);

    // Determine color and shape based on assetName
    Color fillColor = {255, 0, 255, 255}; // Default magenta
    Color outlineColor = {0, 0, 0, 255}; // Default black outline
    bool isCircle = false;

    if (assetName.find("characters") != std::string::npos) {
        fillColor = {0, 150, 255, 255}; // Blue for characters
        isCircle = true;
    } else if (assetName.find("weapons") != std::string::npos) {
        fillColor = {255, 150, 0, 255}; // Orange for weapons
        isCircle = false; // Rectangle
    } else if (assetName.find("projectiles") != std::string::npos) {
        fillColor = {255, 255, 0, 255}; // Yellow for projectiles
        isCircle = true; // Small circle
    }

    // Fill the texture with the determined color
    SDL_SetRenderDrawColor(m_sdlRenderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
    SDL_RenderClear(m_sdlRenderer);

    // Draw shape
    SDL_SetRenderDrawColor(m_sdlRenderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    if (isCircle) {
        // Draw a filled circle in the center
        float centerX = width / 2.0f;
        float centerY = height / 2.0f;
        float radius = std::min(width, height) / 3.0f; // Make it a bit smaller than the texture
        drawCircle(centerX, centerY, radius, fillColor, true); // Filled circle
        drawCircle(centerX, centerY, radius, outlineColor, false); // Outline
    } else {
        // Draw a filled rectangle with an outline
        SDL_FRect rect = {static_cast<float>(width / 4), static_cast<float>(height / 4), static_cast<float>(width / 2), static_cast<float>(height / 2)};
        drawRect(&rect, fillColor, true); // Filled rectangle
        drawRect(&rect, outlineColor, false); // Outline
    }

    // Restore previous rendering target and draw color
    SDL_SetRenderTarget(m_sdlRenderer, nullptr);
    SDL_SetRenderDrawColor(m_sdlRenderer, oldR, oldG, oldB, oldA);

    Log::Info("Generated placeholder texture for '" + assetName + "' (W: " + std::to_string(width) + ", H: " + std::to_string(height) + ").");
    return placeholderTexture;
}

// Placeholder for Map Rendering - requires MapManager implementation details
void Renderer::renderMap(const MapManager& mapManager, MapLayer layer) {
    if (!m_isInitialized || !m_sdlRenderer || !mapManager.isMapLoaded()) return;

    // Get map properties
    unsigned tileWidth = mapManager.getTileWidth();
    unsigned tileHeight = mapManager.getTileHeight();

    // Iterate through all layers in the TMX map
    const auto& tmxLayers = mapManager.getRawLayers();
    for (const auto& tmxLayer : tmxLayers) {
        if (tmxLayer->getType() == tmx::Layer::Type::Tile) {
            const auto& tileLayer = tmxLayer->getLayerAs<tmx::TileLayer>();

            // Determine if this layer should be rendered based on the MapLayer enum
            // This is a simplified check; a more robust system might use layer properties from Tiled
            bool renderThisLayer = false;
            if (layer == MapLayer::Background && tmxLayer->getName().find("background") != std::string::npos) {
                renderThisLayer = true;
            } else if (layer == MapLayer::Foreground && tmxLayer->getName().find("foreground") != std::string::npos) {
                renderThisLayer = true;
            } else if (layer == MapLayer::Objects && tmxLayer->getName().find("object") != std::string::npos) {
                // Object layers are typically handled by EntityManager, but if they contain tiles, render them
                renderThisLayer = true;
            }
            // If no specific layer type is requested, render all tile layers
            if (layer == MapLayer::Background && tmxLayer->getName().find("background") == std::string::npos && tmxLayer->getName().find("foreground") == std::string::npos && tmxLayer->getName().find("object") == std::string::npos) {
                renderThisLayer = true;
            }


            if (renderThisLayer) {
                const auto& tiles = tileLayer.getTiles();
                for (unsigned int y = 0; y < tileLayer.getSize().y; ++y) {
                    for (unsigned int x = 0; x < tileLayer.getSize().x; ++x) {
                        unsigned int tileIndex = x + y * tileLayer.getSize().x;
                        if (tileIndex < tiles.size()) {
                            const auto& tile = tiles[tileIndex];
                            if (tile.ID > 0) { // ID 0 means empty tile
                                const TilesetInfo* tilesetInfo = mapManager.findTilesetForGid(tile.ID);
                                if (tilesetInfo) {
                                    SDL_Texture* tilesetTexture = loadTexture(tilesetInfo->imagePath);
                                    if (tilesetTexture) {
                                        SDL_Rect srcRect = mapManager.getSourceRectForGid(tile.ID, *tilesetInfo);
                                        SDL_FRect dstRect = {
                                            static_cast<float>(x * tileWidth),
                                            static_cast<float>(y * tileHeight),
                                            static_cast<float>(tileWidth),
                                            static_cast<float>(tileHeight)
                                        };

                                        // Apply tile flipping if necessary (Tiled supports horizontal, vertical, diagonal flip)
                                        SDL_RendererFlip flip = SDL_FLIP_NONE;
                                        if (tile.flipFlags & tmx::TileLayer::FlipFlag::Horizontal) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
                                        if (tile.flipFlags & tmx::TileLayer::FlipFlag::Vertical) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_VERTICAL);
                                        // Diagonal flip is more complex and might require rotation + flip, or custom shader

                                        drawTexture(tilesetTexture, &srcRect, &dstRect, 0.0, nullptr, flip);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

GLuint Renderer::getOpenGLTextureID(SDL_Texture* texture) {
    if (!m_sdlRenderer || !texture) {
        return 0; // Invalid texture or renderer
    }

    // SDL_GL_BindTexture returns 0 on success and binds the texture.
    // To get the actual GLuint, we query the currently bound texture.
    float texw, texh; // Not used for getting ID, but required by function signature
    if (SDL_GL_BindTexture(texture, &texw, &texh) == 0) {
        GLint glTextureID = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &glTextureID);
        return static_cast<GLuint>(glTextureID);
    } else {
        Log::Error("Failed to bind SDL_Texture to get OpenGL ID: " + std::string(SDL_GetError()));
        return 0;
    }
}

} // namespace TuxArena