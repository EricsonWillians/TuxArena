#ifndef TUXARENA_PARTICLE_H
#define TUXARENA_PARTICLE_H

#include "TuxArena/Entity.h" // For Vec2 and Color
#include "TuxArena/Renderer.h"
#include <glm/glm.hpp>

namespace TuxArena
{

    enum class ParticleType
    {
        Blood,
        BulletTrail
    };

    struct Particle
    {
        glm::vec2 position;
        glm::vec2 velocity;
        Color color;
        float lifetime;
        float initialLifetime;
        float size;
        ParticleType type;
    };

} // namespace TuxArena

#endif // TUXARENA_PARTICLE_H
