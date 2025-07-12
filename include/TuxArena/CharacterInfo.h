#ifndef TUXARENA_CHARACTER_INFO_H
#define TUXARENA_CHARACTER_INFO_H

#include <string>
#include <SDL2/SDL.h> // For SDL_Texture

namespace TuxArena
{
    struct CharacterInfo
    {
        std::string id; // Unique identifier, e.g., "tux", "gnu"
        std::string name; // Display name, e.g., "Tux", "GNU"
        std::string texturePath; // Path to the character's texture file
        SDL_Texture* texture = nullptr; // Loaded texture
        float health = 100.0f;
        float speed = 200.0f;
        std::string specialAbility = "None"; // Placeholder for ability ID/name
    };

} // namespace TuxArena

#endif // TUXARENA_CHARACTER_INFO_H